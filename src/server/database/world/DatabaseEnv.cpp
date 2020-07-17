/*
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
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

#include "DatabaseEnv.h"
#include "DatabaseWorkerPool.ipp"
#include "Connections/CharacterDatabase.h"
#include "Connections/LoginDatabase.h"
#include "Connections/RGDatabase.h"
#include "Connections/WorldDatabase.h"

DB_API WorldDatabaseWorkerPool WorldDatabase;
DB_API CharacterDatabaseWorkerPool CharacterDatabase;
DB_API LoginDatabaseWorkerPool LoginDatabase;
DB_API RGDatabaseWorkerPool RGDatabase;

template class DB_API DatabaseWorkerPool<LoginDatabaseConnection>;
template class DB_API DatabaseWorkerPool<WorldDatabaseConnection>;
template class DB_API DatabaseWorkerPool<CharacterDatabaseConnection>;
template class DB_API DatabaseWorkerPool<RGDatabaseConnection>;
