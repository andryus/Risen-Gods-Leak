#include "Monitoring/Module/Database.hpp"

using namespace RG::Monitoring;

namespace
{
static const prometheus::Summary::Quantiles QUANTILES = {
        {0.5, 0.05},
        {0.9, 0.01},
        {0.95, 0.01},
        {1.0, 0.01},
};
}

Database::Database() :
        Module("database"),
        family_queries_(AddCounter("queries_total", "Total number of executed queries")),
        family_query_execution_time_(AddSummary("query_execution_time", "Execution time of queries in us")),
        family_query_errors_(AddCounter("query_errors_total", "Total number of SQL errors")),
        family_query_queue_size_(AddGauge("query_queue_size", "Number of queries in the queue")),
        family_query_queue_time_(AddSummary("query_queue_time", "Time a operation is in queue before execution")),
        family_transactions_(AddCounter("transactions_total", "Total number of executed transactions")),
        family_transaction_queries_(AddCounter("transaction_queries_total", "Number of executed queries inside transactions"))
{
    for (auto&& name : {"Logon", "Char", "RG", "World"})
        counters_.emplace(name, new DatabaseCounter(*this, name));
}

Database& Database::Instance()
{
    static Database instance;
    return instance;
}

DatabaseCounter* Database::GetCounters(const std::string& name)
{
    auto& counters = Instance().counters_;

    auto itr = counters.find(name);
    if (itr == counters.end())
        return nullptr;
    return itr->second;
}

QueryTypeCounters::QueryTypeCounters(Database& owner, const std::string& db, uint8 mask)
{
    const std::map<std::string, std::string> labels{
            {"database", db},
            {"execute_type", (mask & MASK_EXECUTE_TYPE) == 0 ? "synch" : "async"},
            {"statement_type", (mask & MASK_STATEMENT_TYPE) == 0 ? "ad-hoc" : "prepared"},
            {"query_type", (mask & MASK_QUERY_TYPE) == 0 ? "select" : "change"}
    };

    queries_total = &owner.family_queries_.Add(labels);
    query_execution_time_ = &owner.family_query_execution_time_.Add(labels, QUANTILES);
}

DatabaseCounter::DatabaseCounter(Database& owner, const std::string& db)
{
    for (std::size_t i = 0; i < query_stats.size(); ++i)
        query_stats[i] = QueryTypeCounters(owner, db, static_cast<uint8>(i));

    const std::map<std::string, std::string> labels{
            {"database", db}
    };

    query_errors_total_          = &owner.family_query_errors_.Add(labels);
    queue_size_                  = &owner.family_query_queue_size_.Add(labels);
    queue_time_                  = &owner.family_query_queue_time_.Add(labels, QUANTILES);

    transactions_total_start_    = &owner.family_transactions_.Add({{"database", db}, {"type", "start"}});
    transactions_total_commit_   = &owner.family_transactions_.Add({{"database", db}, {"type", "commit"}});
    transactions_total_rollback_ = &owner.family_transactions_.Add({{"database", db}, {"type", "rollback"}});
    transaction_queries_         = &owner.family_transaction_queries_.Add(labels);
}

AutoLatencyTimer DatabaseCounter::ReportQuery(bool is_async, bool is_prepared, bool is_change)
{
    const uint8 mask = (static_cast<uint8>(is_async) << 0)
                       | (static_cast<uint8>(is_prepared) << 1)
                       | (static_cast<uint8>(is_change) << 2);

    query_stats[mask].queries_total->Increment();

    return AutoLatencyTimer(query_stats[mask].query_execution_time_);
}

void DatabaseCounter::ReportTransaction(std::size_t size)
{
    transactions_total_start_->Increment();
    transaction_queries_->Increment(size);
}

void DatabaseCounter::ReportTransactionCommit()
{
    transactions_total_commit_->Increment();
}

void DatabaseCounter::ReportTransactionRollback()
{
    transactions_total_rollback_->Increment();
}

void DatabaseCounter::ReportError(uint32 error)
{
    query_errors_total_->Increment();
}

void DatabaseCounter::ReportQueueAdd()
{
    queue_size_->Increment();
}

void DatabaseCounter::ReportQueueRemove(const std::chrono::nanoseconds& duration)
{
    queue_size_->Decrement();
    queue_time_->Observe(std::chrono::duration_cast<std::chrono::microseconds>(duration).count() / 1000.0);
}

