/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/// \addtogroup Trinityd Trinity Daemon
/// @{
/// \file

#include <openssl/opensslv.h>
#include <openssl/crypto.h>
#include <IoContext.h>
#include <boost/asio/deadline_timer.hpp>
#include <boost/program_options.hpp>

#include "Asio/Resolver.h"
#include "Common.h"
#include "DatabaseEnv.h"
#include "AsyncAcceptor.h"
#include "RASession.h"
#include "Configuration/Config.h"
#include "IoContext.h"
#include "OpenSSLCrypto.h"
#include "ProcessPriority.h"
#include "BigNumber.h"
#include "World.h"
#include "MapManager.h"
#include "InstanceSaveMgr.h"
#include "ObjectAccessor.h"
#include "ScriptMgr.h"
#include "ScriptLoader.h"
#include "OutdoorPvP/OutdoorPvPMgr.h"
#include "BattlegroundMgr.h"
#include "TCSoap.h"
#include "CliRunnable.h"
#include "GitRevision.h"
#include "WorldSocket.h"
#include "WorldSocketMgr.h"
#include "Realm/Realm.h"
#include "DatabaseLoader.h"
#include "ObjectMgr.h"
#include "Threading/ThreadName.hpp"
#include "Graylog/Graylog.hpp"
#include "Monitoring/Monitoring.hpp"
#include "Monitoring/Module/Database.hpp"
#include "Monitoring/Module/Network.hpp"

#include "ThreadsafetyDebugger.h"
#include "GameTime.h"

#ifdef SYSTEMD_SOCKET_ACTIVATION
#include <systemd/sd-daemon.h>
#endif

#ifndef ENABLE_SCRIPT_RELOAD
#include "RegisteredScripts.h"
#else
#include "ScriptReloader.h"
#endif

using namespace boost::program_options;

#ifndef _TRINITY_CORE_CONFIG
    #define _TRINITY_CORE_CONFIG  "worldserver.conf"
#endif

#define WORLD_SLEEP_CONST 50

#ifdef _WIN32
#include "ServiceWin32.h"
char serviceName[] = "worldserver";
char serviceLongName[] = "TrinityCore world service";
char serviceDescription[] = "TrinityCore World of Warcraft emulator world service";
/*
 * -1 - not in service mode
 *  0 - stopped
 *  1 - running
 *  2 - paused
 */
int m_ServiceStatus = -1;
#endif

std::unique_ptr<boost::asio::deadline_timer> _freezeCheckTimer;
uint32 _worldLoopCounter(0);
uint32 _lastChangeMsTime(0);
uint32 _maxCoreStuckTimeInMs(0);

void SignalHandler(const boost::system::error_code& error, int signalNumber);
void FreezeDetectorHandler(const boost::system::error_code& error);
std::unique_ptr<AsyncAcceptor> StartRaSocketAcceptor(Trinity::Asio::IoContext& ioContext);
bool StartDB();
void StopDB();
void WorldUpdateLoop();
void ClearOnlineAccounts();
void ShutdownCLIThread(std::thread* cliThread);
bool LoadRealmInfo(Trinity::Asio::IoContext& context);
variables_map GetConsoleArguments(int argc, char** argv, std::string& cfg_file, std::string& cfg_service);

class IOContextUpdater
{
public:
    explicit IOContextUpdater(const std::string& name) :
            context_(),
            work_(context_),
            thread_(&IOContextUpdater::Run, this, name),
            stopped_(false)
    {}

    void stop()
    {
        context_.stop();
        thread_.join();

        stopped_ = true;
    }

    ~IOContextUpdater()
    {
        if (!stopped_)
            stop();
    }

    Trinity::Asio::IoContext& GetContext() { return context_; }

private:
    std::size_t Run(std::string name)
    {
        util::thread::nameing::SetName(name);
        return context_.run();
    }

private:
    Trinity::Asio::IoContext context_;
    Trinity::Asio::IoContext::work work_;
    std::thread thread_;
    bool stopped_;
};

/// Launch the Trinity server
extern int main(int argc, char** argv)
{
#ifdef SYSTEMD_SOCKET_ACTIVATION
    sd_notifyf(0, "STATUS=World Starting...\n");
#endif

    ThreadsafetyDebugger::Allow(ThreadsafetyDebugger::Context::NEEDS_WORLD);
    ThreadsafetyDebugger::Allow(ThreadsafetyDebugger::Context::NEEDS_MAP);

    signal(SIGABRT, &Trinity::AbortHandler);

    std::string configFile = _TRINITY_CORE_CONFIG;
    std::string configService;

    auto vm = GetConsoleArguments(argc, argv, configFile, configService);
    // exit if help is enabled
    if (vm.count("help"))
        return 0;

#ifdef _WIN32
    if (configService.compare("install") == 0)
        return WinServiceInstall() == true ? 0 : 1;
    else if (configService.compare("uninstall") == 0)
        return WinServiceUninstall() == true ? 0 : 1;
    else if (configService.compare("run") == 0)
        WinServiceRun();
#endif

    std::string configError;
    if (!sConfigMgr->LoadInitial(configFile, configError))
    {
        printf("Error in config file: %s\n", configError.c_str());
        return 1;
    }

    // freeze detector
    IOContextUpdater freezeContext{"FreezeDetector"};
    _freezeCheckTimer = std::make_unique<boost::asio::deadline_timer>(freezeContext.GetContext());

    IOContextUpdater logContext{"LogWriter"};

    // Set signal handlers (this must be done before starting IoContext threads, because otherwise they would unblock and exit)
    boost::asio::signal_set signals(logContext.GetContext(), SIGINT, SIGTERM);
#if PLATFORM == PLATFORM_WINDOWS
    signals.add(SIGBREAK);
#endif
    signals.async_wait(SignalHandler);

    // If logs are supposed to be handled async then we need to pass the IoContext into the Log singleton
    sLog->Initialize(sConfigMgr->GetBoolDefault("Log.Async.Enable", false) ? &logContext.GetContext() : nullptr);

    sGraylog->Initialize();
#ifdef CONFIG_WITH_PROMETHEUS_MONITORING
    RG::Monitoring::Initialize();
    RG::Monitoring::RegisterModule<RG::Monitoring::Network>();
    RG::Monitoring::RegisterModule<RG::Monitoring::Database>();
#endif

    TC_LOG_INFO("server.worldserver", "%s (worldserver-daemon)", GitRevision::GetFullVersion());
    TC_LOG_INFO("server.worldserver", "<Ctrl-C> to stop.\n");
    TC_LOG_INFO("server.worldserver", " ______                       __");
    TC_LOG_INFO("server.worldserver", "/\\__  _\\       __          __/\\ \\__");
    TC_LOG_INFO("server.worldserver", "\\/_/\\ \\/ _ __ /\\_\\    ___ /\\_\\ \\, _\\  __  __");
    TC_LOG_INFO("server.worldserver", "   \\ \\ \\/\\`'__\\/\\ \\ /' _ `\\/\\ \\ \\ \\/ /\\ \\/\\ \\");
    TC_LOG_INFO("server.worldserver", "    \\ \\ \\ \\ \\/ \\ \\ \\/\\ \\/\\ \\ \\ \\ \\ \\_\\ \\ \\_\\ \\");
    TC_LOG_INFO("server.worldserver", "     \\ \\_\\ \\_\\  \\ \\_\\ \\_\\ \\_\\ \\_\\ \\__\\\\/`____ \\");
    TC_LOG_INFO("server.worldserver", "      \\/_/\\/_/   \\/_/\\/_/\\/_/\\/_/\\/__/ `/___/> \\");
    TC_LOG_INFO("server.worldserver", "                                 C O R E  /\\___/");
    TC_LOG_INFO("server.worldserver", "http://TrinityCore.org                    \\/__/\n");
    TC_LOG_INFO("server.worldserver", "Using configuration file %s.", configFile.c_str());
    TC_LOG_INFO("server.worldserver", "Using SSL version: %s (library: %s)", OPENSSL_VERSION_TEXT, SSLeay_version(SSLEAY_VERSION));
    TC_LOG_INFO("server.worldserver", "Using Boost version: %i.%i.%i", BOOST_VERSION / 100000, BOOST_VERSION / 100 % 1000, BOOST_VERSION % 100);

    OpenSSLCrypto::threadsSetup();

    // Seed the OpenSSL's PRNG here.
    // That way it won't auto-seed when calling BigNumber::SetRand and slow down the first world login
    BigNumber seed;
    seed.SetRand(16 * 8);

    /// load new scripts
#ifndef ENABLE_SCRIPT_RELOAD
    registerScripts();
#else
    sScriptReloader.Init();
    sScriptReloader.Load("script_content");
#endif

    /// worldserver PID file creation
    std::string pidFile = sConfigMgr->GetStringDefault("PidFile", "");
    if (!pidFile.empty())
    {
        if (uint32 pid = CreatePIDFile(pidFile))
            TC_LOG_INFO("server.worldserver", "Daemon PID: %u\n", pid);
        else
        {
            TC_LOG_ERROR("server.worldserver", "Cannot create PID file %s.\n", pidFile.c_str());
            return 1;
        }
    }

    // Set process priority according to configuration settings
    SetProcessPriority("server.worldserver");

    // Start the databases
    if (!StartDB())
        return 1;

    // Set server offline (not connectable)
    LoginDatabase.DirectPExecute("UPDATE realmlist SET flag = flag | %u WHERE id = '%d'", REALM_FLAG_OFFLINE, realm.Id.Realm);

    IOContextUpdater acceptorContext{"Acceptor"};

    LoadRealmInfo(acceptorContext.GetContext());

    // Initialize the World
    sScriptMgr->SetScriptLoader(AddScripts);
    sWorld->SetInitialWorldSettings();

    // Launch CliRunnable thread
    std::thread* cliThread = nullptr;
#ifdef _WIN32
    if (sConfigMgr->GetBoolDefault("Console.Enable", true) && (m_ServiceStatus == -1)/* need disable console in service mode*/)
#else
    if (sConfigMgr->GetBoolDefault("Console.Enable", true))
#endif
    {
        cliThread = new std::thread(CliThread);
    }

    // Start the Remote Access port (acceptor) if enabled
    std::unique_ptr<AsyncAcceptor> raAcceptor;
    if (sConfigMgr->GetBoolDefault("Ra.Enable", false))
        raAcceptor = StartRaSocketAcceptor(acceptorContext.GetContext());

    // Start soap serving thread if enabled
    std::thread* soapThread = nullptr;
    if (sConfigMgr->GetBoolDefault("SOAP.Enabled", false))
    {
        soapThread = new std::thread(TCSoapThread, sConfigMgr->GetStringDefault("SOAP.IP", "127.0.0.1"), uint16(sConfigMgr->GetIntDefault("SOAP.Port", 7878)));
    }

    // Launch the worldserver listener socket
    uint16 worldPort = uint16(sWorld->getIntConfig(CONFIG_PORT_WORLD));
    std::string worldListener = sConfigMgr->GetStringDefault("BindIP", "0.0.0.0");

    int networkThreads = sConfigMgr->GetIntDefault("Network.Threads", 1);

    if (networkThreads <= 0)
    {
        TC_LOG_ERROR("server.worldserver", "Network.Threads must be greater than 0");
        return ERROR_EXIT_CODE;
    }

#ifdef SYSTEMD_SOCKET_ACTIVATION
    // sd_listen_fds checks if LISTEN_PID == getpid() but if we are running in gdb LISTEN_PID is the pid of the gdb process
    // therefore it thinks that we do not have any inherited fds
    // the fd gets passed correctly anyway, so just change LISTEN_PID to the correct value
    {
        char envString[255];
        std::snprintf(envString, 255, "LISTEN_PID=%d", getpid());
        putenv(envString);
    }
    int fdCount = sd_listen_fds(0);
    if (fdCount != 1)
    {
        std::unique_ptr<AsyncAcceptor> worldAcceptor(new AsyncAcceptor(acceptorContext.GetContext(), worldListener, worldPort));
        if (!sWorldSocketMgr.StartNetwork(std::move(worldAcceptor), networkThreads))
        {
            TC_LOG_ERROR("server.worldserver", "Failed to initialize network");
            return ERROR_EXIT_CODE;
        }
    }
    else
    {
        if (!sd_is_socket(SD_LISTEN_FDS_START, AF_INET, SOCK_STREAM, 1))
        {
            TC_LOG_FATAL("server.worldserver", "Activated with invalid fd");
            return ERROR_EXIT_CODE;
        }

        std::unique_ptr<AsyncAcceptor> worldAcceptor(new AsyncAcceptor(acceptorContext.GetContext(), SD_LISTEN_FDS_START));
        if (!sWorldSocketMgr.StartNetwork(std::move(worldAcceptor), networkThreads))
        {
            TC_LOG_ERROR("server.worldserver", "Failed to initialize network");
            return ERROR_EXIT_CODE;
        }
        sWorld->ShutdownServ(5*60, SHUTDOWN_MASK_IDLE, SHUTDOWN_EXIT_CODE, std::string("Socket Activation Shutdown"));

    }
#else
    std::unique_ptr<AsyncAcceptor> worldAcceptor(new AsyncAcceptor(acceptorContext.GetContext(), worldListener, worldPort));
    if (!sWorldSocketMgr.StartNetwork(std::move(worldAcceptor), networkThreads))
    {
        TC_LOG_ERROR("server.worldserver", "Failed to initialize network");
        return ERROR_EXIT_CODE;
    }
#endif

    // Set server online (allow connecting now)
    LoginDatabase.DirectPExecute("UPDATE realmlist SET flag = flag & ~%u, population = 0 WHERE id = '%u'", REALM_FLAG_OFFLINE, realm.Id.Realm);
    realm.PopulationLevel = 0.0f;
    realm.Flags = RealmFlags(realm.Flags & ~uint32(REALM_FLAG_OFFLINE));

    // Start the freeze check callback cycle in 5 seconds (cycle itself is 1 sec)
    if (int coreStuckTime = sConfigMgr->GetIntDefault("MaxCoreStuckTime", 0))
    {
        _maxCoreStuckTimeInMs = coreStuckTime * 1000;
        _freezeCheckTimer->expires_from_now(boost::posix_time::seconds(5));
        _freezeCheckTimer->async_wait(FreezeDetectorHandler);
        TC_LOG_INFO("server.worldserver", "Starting up anti-freeze thread (%u seconds max stuck time)...", coreStuckTime);
    }

    TC_LOG_INFO("server.worldserver", "%s (worldserver-daemon) ready...", GitRevision::GetFullVersion());

#ifdef SYSTEMD_SOCKET_ACTIVATION
    sd_notifyf(0, "READY=1\n"
                  "STATUS=World Started...\n"
               );
#endif

    sScriptMgr->OnStartup();

    WorldUpdateLoop();

#ifdef SYSTEMD_SOCKET_ACTIVATION
    sd_notifyf(0, "STOPPING=1\n"
                  "STATUS=Shutting Down\n"
               );
#endif

    logContext.stop();
    freezeContext.stop();

    sLog->SetSynchronous();

    sWorld->KickAll();                                       // save and kick all players
    sWorld->UpdateSessions(1);                             // real players unload required UpdateSessions call
    sObjectMgr->GetItemIdGenerator().Shutdown();   // stop id loader thread

    sScriptMgr->OnShutdown();

    // unload battleground templates before different singletons destroyed
    sBattlegroundMgr->DeleteAllBattlegrounds();

    sWorldSocketMgr.StopNetwork();

    sInstanceSaveMgr->Unload();
    sMapMgr->UnloadAll();                     // unload all grids (including locked in memory)
    sObjectAccessor->UnloadAll();             // unload 'i_player2corpse' storage and remove from world
    sScriptMgr->Unload();
    sOutdoorPvPMgr->Die();

    sWorld->normalThreadPool.stop();
    sWorld->lowPrioThreadPool.stop();

#ifdef SYSTEMD_SOCKET_ACTIVATION
    if (fdCount != 1)
        LoginDatabase.DirectPExecute("UPDATE realmlist SET flag = flag | %u WHERE id = '%d'", REALM_FLAG_OFFLINE, realm.Id.Realm);
#else

    // set server offline
    LoginDatabase.DirectPExecute("UPDATE realmlist SET flag = flag | %u WHERE id = '%d'", REALM_FLAG_OFFLINE, realm.Id.Realm);
#endif

    // Clean up threads if any
    if (soapThread != nullptr)
    {
        soapThread->join();
        delete soapThread;
    }

    if (raAcceptor)
        raAcceptor->Close();

    acceptorContext.stop();

    ///- Clean database before leaving
    ClearOnlineAccounts();

    StopDB();

    TC_LOG_INFO("server.worldserver", "Halting process...");

    ShutdownCLIThread(cliThread);

    OpenSSLCrypto::threadsCleanup();

    // 0 - normal shutdown
    // 1 - shutdown at error
    // 2 - restart command used, this code can be used by restarter for restart Trinityd


#ifdef SYSTEMD_SOCKET_ACTIVATION
    sd_notifyf(0, "STATUS=Exit\n");
#endif

    return World::GetExitCode();
}

void ShutdownCLIThread(std::thread* cliThread)
{
    if (cliThread != nullptr)
    {
#ifdef _WIN32
        // First try to cancel any I/O in the CLI thread
        if (!CancelSynchronousIo(cliThread->native_handle()))
        {
            // if CancelSynchronousIo() fails, print the error and try with old way
            DWORD errorCode = GetLastError();
            LPSTR errorBuffer;

            DWORD formatReturnCode = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
                                                   nullptr, errorCode, 0, (LPTSTR)&errorBuffer, 0, nullptr);
            if (!formatReturnCode)
                errorBuffer = "Unknown error";

            TC_LOG_DEBUG("server.worldserver", "Error cancelling I/O of CliThread, error code %u, detail: %s",
                uint32(errorCode), errorBuffer);
            LocalFree(errorBuffer);

            // send keyboard input to safely unblock the CLI thread
            INPUT_RECORD b[4];
            HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
            b[0].EventType = KEY_EVENT;
            b[0].Event.KeyEvent.bKeyDown = TRUE;
            b[0].Event.KeyEvent.uChar.AsciiChar = 'X';
            b[0].Event.KeyEvent.wVirtualKeyCode = 'X';
            b[0].Event.KeyEvent.wRepeatCount = 1;

            b[1].EventType = KEY_EVENT;
            b[1].Event.KeyEvent.bKeyDown = FALSE;
            b[1].Event.KeyEvent.uChar.AsciiChar = 'X';
            b[1].Event.KeyEvent.wVirtualKeyCode = 'X';
            b[1].Event.KeyEvent.wRepeatCount = 1;

            b[2].EventType = KEY_EVENT;
            b[2].Event.KeyEvent.bKeyDown = TRUE;
            b[2].Event.KeyEvent.dwControlKeyState = 0;
            b[2].Event.KeyEvent.uChar.AsciiChar = '\r';
            b[2].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
            b[2].Event.KeyEvent.wRepeatCount = 1;
            b[2].Event.KeyEvent.wVirtualScanCode = 0x1c;

            b[3].EventType = KEY_EVENT;
            b[3].Event.KeyEvent.bKeyDown = FALSE;
            b[3].Event.KeyEvent.dwControlKeyState = 0;
            b[3].Event.KeyEvent.uChar.AsciiChar = '\r';
            b[3].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
            b[3].Event.KeyEvent.wVirtualScanCode = 0x1c;
            b[3].Event.KeyEvent.wRepeatCount = 1;
            DWORD numb;
            WriteConsoleInput(hStdIn, b, 4, &numb);
        }
#endif
        cliThread->join();
        delete cliThread;
    }
}

void WorldUpdateLoop()
{
    uint32 realCurrTime = 0;
    uint32 realPrevTime = getMSTime();

    uint32 prevSleepTime = 0;                               // used for balanced full tick time length near WORLD_SLEEP_CONST

#ifdef SYSTEMD_SOCKET_ACTIVATION
    TimeTrackerSmall systemdUpdateTracker(1000);
#endif

    ///- While we have not World::m_stopEvent, update the world
    while (!World::IsStopped())
    {
        ++World::m_worldLoopCounter;
        realCurrTime = getMSTime();

        uint32 diff = getMSTimeDiff(realPrevTime, realCurrTime);


#ifdef SYSTEMD_SOCKET_ACTIVATION
        systemdUpdateTracker.Update(diff);
        if (systemdUpdateTracker.Passed())
        {
            systemdUpdateTracker.Reset(1000);
            sd_notifyf(0, "WATCHDOG=1\n"
                          "STATUS=World Running. Players: %d Clients: %d Uptime: %s\n"
                       , sWorld->GetPlayerCount(), sWorld->GetActiveSessionCount(), secsToTimeString(game::time::GetUptime()).c_str()
                       );
        }
#endif

        sWorld->Update(diff);
        realPrevTime = realCurrTime;

        // diff (D0) include time of previous sleep (d0) + tick time (t0)
        // we want that next d1 + t1 == WORLD_SLEEP_CONST
        // we can't know next t1 and then can use (t0 + d1) == WORLD_SLEEP_CONST requirement
        // d1 = WORLD_SLEEP_CONST - t0 = WORLD_SLEEP_CONST - (D0 - d0) = WORLD_SLEEP_CONST + d0 - D0
        if (diff <= WORLD_SLEEP_CONST + prevSleepTime)
        {
            prevSleepTime = WORLD_SLEEP_CONST + prevSleepTime - diff;

            std::this_thread::sleep_for(std::chrono::milliseconds(prevSleepTime));
        }
        else
            prevSleepTime = 0;

#ifdef _WIN32
        if (m_ServiceStatus == 0)
            World::StopNow(SHUTDOWN_EXIT_CODE);

        while (m_ServiceStatus == 2)
            Sleep(1000);
#endif
    }
}

void SignalHandler(const boost::system::error_code& error, int /*signalNumber*/)
{
    if (!error)
        World::StopNow(SHUTDOWN_EXIT_CODE);
}

void FreezeDetectorHandler(const boost::system::error_code& error)
{
    if (!error)
    {
        uint32 curtime = getMSTime();

        uint32 worldLoopCounter = World::m_worldLoopCounter;
        if (_worldLoopCounter != worldLoopCounter)
        {
            _lastChangeMsTime = curtime;
            _worldLoopCounter = worldLoopCounter;
        }
        // possible freeze
        else if (getMSTimeDiff(_lastChangeMsTime, curtime) > _maxCoreStuckTimeInMs)
        {
            TC_LOG_ERROR("server.worldserver", "World Thread hangs, kicking out server!");
            ABORT();
        }

        _freezeCheckTimer->expires_from_now(boost::posix_time::seconds(1));
        _freezeCheckTimer->async_wait(FreezeDetectorHandler);
    }
}

std::unique_ptr<AsyncAcceptor> StartRaSocketAcceptor(Trinity::Asio::IoContext& ioContext)
{
    uint16 raPort = uint16(sConfigMgr->GetIntDefault("Ra.Port", 3443));
    std::string raListener = sConfigMgr->GetStringDefault("Ra.IP", "0.0.0.0");

    AsyncAcceptor* acceptor = new AsyncAcceptor(ioContext, raListener, raPort);
    acceptor->AsyncAccept<RASession>();
    return std::unique_ptr<AsyncAcceptor>{acceptor};
}

bool LoadRealmInfo(Trinity::Asio::IoContext& context)
{
    QueryResult result = LoginDatabase.PQuery("SELECT id, name, address, localAddress, localSubnetMask, port, icon, flag, timezone, allowedSecurityLevel, population, gamebuild FROM realmlist WHERE id = %u", realm.Id.Realm);
    if (!result)
        return false;

    boost::asio::ip::tcp::resolver resolver(context);

    Field* fields = result->Fetch();
    realm.Name = fields[1].GetString();
    Optional<boost::asio::ip::tcp::endpoint> externalAddress = Trinity::Net::Resolve(resolver, boost::asio::ip::tcp::v4(), fields[2].GetString(), "");
    if (!externalAddress)
    {
        TC_LOG_ERROR("server.worldserver", "Could not resolve address %s", fields[2].GetString().c_str());
        return false;
    }

    realm.ExternalAddress = externalAddress->address();

    Optional<boost::asio::ip::tcp::endpoint> localAddress = Trinity::Net::Resolve(resolver, boost::asio::ip::tcp::v4(), fields[3].GetString(), "");
    if (!localAddress)
    {
        TC_LOG_ERROR("server.worldserver", "Could not resolve address %s", fields[3].GetString().c_str());
        return false;
    }

    realm.LocalAddress = localAddress->address();

    Optional<boost::asio::ip::tcp::endpoint> localSubmask = Trinity::Net::Resolve(resolver, boost::asio::ip::tcp::v4(), fields[4].GetString(), "");
    if (!localSubmask)
    {
        TC_LOG_ERROR("server.worldserver", "Could not resolve address %s", fields[4].GetString().c_str());
        return false;
    }

    realm.LocalSubnetMask = localSubmask->address();

    realm.Port = fields[5].GetUInt16();
    realm.Type = fields[6].GetUInt8();
    realm.Flags = RealmFlags(fields[7].GetUInt8());
    realm.Timezone = fields[8].GetUInt8();
    realm.AllowedSecurityLevel = AccountTypes(fields[9].GetUInt8());
    realm.PopulationLevel = fields[10].GetFloat();
    realm.Build = fields[11].GetUInt32();
    return true;
}

/// Initialize connection to the databases
bool StartDB()
{
    MySQL::Library_Init();

    // Load databases
    DatabaseLoader loader("server.worldserver");
    loader
        .AddDatabase(LoginDatabase, "Login")
        .AddDatabase(CharacterDatabase, "Character")
        .AddDatabase(WorldDatabase, "World");

    if (sConfigMgr->GetBoolDefault("RG.Database", false))
    {
        TC_LOG_INFO("server.worldserver", "Activating RG Database connection...");
        loader.AddDatabase(RGDatabase, "RG");
    }
    else
        TC_LOG_INFO("server.worldserver", "RG Database connection is disabled");

    if (!loader.Load())
        return false;

    ///- Get the realm Id from the configuration file
    realm.Id.Realm = sConfigMgr->GetIntDefault("RealmID", 0);
    if (!realm.Id.Realm)
    {
        TC_LOG_ERROR("server.worldserver", "Realm ID not defined in configuration file");
        return false;
    }

    TC_LOG_INFO("server.worldserver", "Realm running as realm ID %d", realm.Id.Realm);

    ///- Clean the database before starting
    ClearOnlineAccounts();

    ///- Insert version info into DB
    WorldDatabase.PExecute("UPDATE version SET core_version = '%s', core_revision = '%s'", GitRevision::GetFullVersion(), GitRevision::GetHash());        // One-time query

    sWorld->LoadDBVersion();

    TC_LOG_INFO("server.worldserver", "Using World DB: %s", sWorld->GetDBVersion());
    return true;
}

void StopDB()
{
    CharacterDatabase.Close();
    WorldDatabase.Close();
    LoginDatabase.Close();
    if (sConfigMgr->GetBoolDefault("RG.Database", false))
        RGDatabase.Close();

    MySQL::Library_End();
}

/// Clear 'online' status for all accounts with characters in this realm
void ClearOnlineAccounts()
{
    // Reset online status for all accounts with characters on the current realm
    LoginDatabase.DirectPExecute("UPDATE account SET online = 0 WHERE online > 0 AND id IN (SELECT acctid FROM realmcharacters WHERE realmid = %d)", realm.Id.Realm);

    // Reset online status for all characters
    CharacterDatabase.DirectExecute("UPDATE characters SET online = 0 WHERE online <> 0");

    // Battleground instance ids reset at server restart
    CharacterDatabase.DirectExecute("UPDATE character_battleground_data SET instanceId = 0");
}

/// @}

variables_map GetConsoleArguments(int argc, char** argv, std::string& configFile, std::string& configService)
{
    // Silences warning about configService not be used if the OS is not Windows
    (void)configService;

    options_description all("Allowed options");
    all.add_options()
        ("help,h", "print usage message")
        ("config,c", value<std::string>(&configFile)->default_value(_TRINITY_CORE_CONFIG), "use <arg> as configuration file")
        ;
    variables_map vm;
    try
    {
        store(command_line_parser(argc, argv).options(all).allow_unregistered().run(), vm);
        notify(vm);
    }
    catch (std::exception& e) {
        std::cerr << e.what() << "\n";
    }

    if (vm.count("help")) {
        std::cout << all << "\n";
    }

    return vm;
}
