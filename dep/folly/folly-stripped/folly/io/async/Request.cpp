/*
 * Copyright 2004-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <folly/io/async/Request.h>

namespace folly {
/**
 * Given a map and a key, return a reference to the value corresponding to the
 * key in the map, or the given default reference if the key doesn't exist in
 * the map.
 */
template <class Map, typename Key = typename Map::key_type>
const typename Map::mapped_type& get_ref_default(
        const Map& map,
        const Key& key,
        const typename Map::mapped_type& dflt) {
  auto pos = map.find(key);
  return (pos != map.end() ? pos->second : dflt);
}

/**
 * Passing a temporary default value returns a dangling reference when it is
 * returned. Lifetime extension is broken by the indirection.
 * The caller must ensure that the default value outlives the reference returned
 * by get_ref_default().
 */
template <class Map, typename Key = typename Map::key_type>
const typename Map::mapped_type& get_ref_default(
        const Map& map,
        const Key& key,
        typename Map::mapped_type&& dflt) = delete;

template <class Map, typename Key = typename Map::key_type>
const typename Map::mapped_type& get_ref_default(
        const Map& map,
        const Key& key,
        const typename Map::mapped_type&& dflt) = delete;

/**
 * Given a map and a key, return a reference to the value corresponding to the
 * key in the map, or the given default reference if the key doesn't exist in
 * the map.
 */
template <
        class Map,
        typename Key = typename Map::key_type,
        typename Func,
        typename = typename std::enable_if<std::is_convertible<
                typename std::result_of<Func()>::type,
                const typename Map::mapped_type&>::value>::type,
        typename = typename std::enable_if<
                std::is_reference<typename std::result_of<Func()>::type>::value>::type>
const typename Map::mapped_type&
get_ref_default(const Map& map, const Key& key, Func&& dflt) {
  auto pos = map.find(key);
  return (pos != map.end() ? pos->second : dflt());
}
}

namespace folly {

void RequestContext::setContextData(
    const std::string& val,
    std::unique_ptr<RequestData> data) {
  auto wlock = data_.wlock();
  if (wlock->count(val)) {
//    LOG_FIRST_N(WARNING, 1)
//        << "Called RequestContext::setContextData with data already set";

    (*wlock)[val] = nullptr;
  } else {
    (*wlock)[val] = std::move(data);
  }
}

bool RequestContext::setContextDataIfAbsent(
    const std::string& val,
    std::unique_ptr<RequestData> data) {
  auto ulock = data_.ulock();
  if (ulock->count(val)) {
    return false;
  }

  auto wlock = ulock.moveFromUpgradeToWrite();
  (*wlock)[val] = std::move(data);
  return true;
}

bool RequestContext::hasContextData(const std::string& val) const {
  return data_.rlock()->count(val);
}

RequestData* RequestContext::getContextData(const std::string& val) {
  const std::unique_ptr<RequestData> dflt{nullptr};
  return get_ref_default(*data_.rlock(), val, dflt).get();
}

const RequestData* RequestContext::getContextData(
    const std::string& val) const {
  const std::unique_ptr<RequestData> dflt{nullptr};
  return get_ref_default(*data_.rlock(), val, dflt).get();
}

void RequestContext::onSet() {
  auto rlock = data_.rlock();
  for (auto const& ent : *rlock) {
    if (auto& data = ent.second) {
      data->onSet();
    }
  }
}

void RequestContext::onUnset() {
  auto rlock = data_.rlock();
  for (auto const& ent : *rlock) {
    if (auto& data = ent.second) {
      data->onUnset();
    }
  }
}

void RequestContext::clearContextData(const std::string& val) {
  std::unique_ptr<RequestData> requestData;
  // Delete the RequestData after giving up the wlock just in case one of the
  // RequestData destructors will try to grab the lock again.
  {
    auto wlock = data_.wlock();
    auto it = wlock->find(val);
    if (it != wlock->end()) {
      requestData = std::move(it->second);
      wlock->erase(it);
    }
  }
}

std::shared_ptr<RequestContext> RequestContext::setContext(
    std::shared_ptr<RequestContext> ctx) {
  auto& curCtx = getStaticContext();
  if (ctx != curCtx) {
//    FOLLY_SDT(folly, request_context_switch_before, curCtx.get(), ctx.get());
    using std::swap;
    if (curCtx) {
      curCtx->onUnset();
    }
    swap(ctx, curCtx);
    if (curCtx) {
      curCtx->onSet();
    }
  }
  return ctx;
}

std::shared_ptr<RequestContext>& RequestContext::getStaticContext() {
  static thread_local std::shared_ptr<RequestContext> singleton;

  return singleton;
}

RequestContext* RequestContext::get() {
  auto context = getStaticContext();
  if (!context) {
    static RequestContext defaultContext;
    return std::addressof(defaultContext);
  }
  return context.get();
}
}
