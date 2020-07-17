#pragma once

#ifdef CONFIG_WITH_PROMETHEUS_MONITORING
#include "Monitoring/Module.hpp"
#include "Monitoring/AutoLatencyTimer.hpp"

namespace RG { namespace Monitoring {

class QueryTypeCounters;
class DatabaseCounter;

class Database : public Module
{
    friend class QueryTypeCounters;
    friend class DatabaseCounter;
public:
    static Database& Instance();
    static DatabaseCounter* GetCounters(const std::string& name);

private:
    Database();

private:
    prometheus::Family<prometheus::Counter>& family_queries_;
    prometheus::Family<prometheus::Summary>& family_query_execution_time_;
    prometheus::Family<prometheus::Counter>& family_query_errors_;
    prometheus::Family<prometheus::Gauge>&   family_query_queue_size_;
    prometheus::Family<prometheus::Summary>& family_query_queue_time_;


    prometheus::Family<prometheus::Counter>&   family_transactions_;
    prometheus::Family<prometheus::Counter>&   family_transaction_queries_;

    std::unordered_map<std::string, DatabaseCounter*> counters_;
};

class QueryTypeCounters
{
    static constexpr uint8 MASK_EXECUTE_TYPE   = 0b001;
    static constexpr uint8 MASK_STATEMENT_TYPE = 0b010;
    static constexpr uint8 MASK_QUERY_TYPE     = 0b100;

public:
    QueryTypeCounters() = default;
    QueryTypeCounters(Database& owner, const std::string& db, uint8 mask);

    prometheus::Counter* queries_total;
    prometheus::Summary* query_execution_time_;
};

class DatabaseCounter
{
    friend class Database;
public:
    AutoLatencyTimer ReportQuery(bool is_async, bool is_prepared, bool is_change);
    void ReportError(uint32 error);

    void ReportTransaction(std::size_t size);
    void ReportTransactionRollback();
    void ReportTransactionCommit();

    void ReportQueueAdd();
    void ReportQueueRemove(const std::chrono::nanoseconds& duration);

private:
    DatabaseCounter(Database& owner, const std::string& db);

    std::array<QueryTypeCounters, 8> query_stats;

    prometheus::Counter* query_errors_total_;
    prometheus::Gauge*   queue_size_;
    prometheus::Summary* queue_time_;

    prometheus::Counter* transactions_total_start_;
    prometheus::Counter* transactions_total_commit_;
    prometheus::Counter* transactions_total_rollback_;
    prometheus::Counter* transaction_queries_;
};

}}

#define MONITOR_DATABASE(WHAT) ::RG::Monitoring::Database::WHAT
#else
#define MONITOR_DATABASE(WHAT)
#endif
