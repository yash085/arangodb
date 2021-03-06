//////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2017 EMC Corporation
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is EMC Corporation
///
/// @author Andrey Abramov
/// @author Vasiliy Nabatchikov
////////////////////////////////////////////////////////////////////////////////

#include "utils/index_utils.hpp"
#include "utils/locale_utils.hpp"

#include "VelocyPackHelper.h"
#include "Basics/StringUtils.h"
#include "velocypack/Builder.h"
#include "velocypack/Iterator.h"
#include "velocypack/Parser.h"
#include "VocBase/LogicalView.h"

#include "IResearchViewMeta.h"

NS_LOCAL

const std::string POLICY_BYTES = "bytes"; // {threshold} > segment_bytes / (all_segment_bytes / #segments)
const std::string POLICY_BYTES_ACCUM = "bytes_accum"; // {threshold} > (segment_bytes + sum_of_merge_candidate_segment_bytes) / all_segment_bytes
const std::string POLICY_DOCS_FILL = "fill"; // {threshold} > #segment_docs{valid} / (#segment_docs{valid} + #segment_docs{removed})
const std::string POLICY_DOCS_LIVE = "count"; // {threshold} > segment_docs{valid} / (all_segment_docs{valid} / #segments)
const std::string POLICY_TIER = "tier"; // scoring policy based on byte size and live docs

template<typename T>
arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy createConsolidationPolicy(
    arangodb::velocypack::Slice const& slice,
    std::string& errorField
);

template<>
arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy createConsolidationPolicy<
    irs::index_utils::consolidate_bytes
>(
    arangodb::velocypack::Slice const& slice,
    std::string& errorField
) {
  irs::index_utils::consolidate_bytes options;
  arangodb::velocypack::Builder properties;

  {
    // optional float
    static const std::string fieldName("threshold");

    if (slice.hasKey(fieldName)) {
      auto field = slice.get(fieldName);

      if (!field.isNumber<float>()) {
        errorField = fieldName;

        return arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy();
      }

      options.threshold = field.getNumber<float>();

      if (options.threshold  < 0. || options.threshold  > 1.) {
        errorField = fieldName;

        return arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy();
      }
    }
  }

  properties.openObject();
  properties.add("type", arangodb::iresearch::toValuePair(POLICY_BYTES));
  properties.add("threshold", arangodb::velocypack::Value(options.threshold));
  properties.close();

  return arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy{
    irs::index_utils::consolidation_policy(options), std::move(properties)
  };
}

template<>
arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy createConsolidationPolicy<
    irs::index_utils::consolidate_bytes_accum
>(
    arangodb::velocypack::Slice const& slice,
    std::string& errorField
) {
  irs::index_utils::consolidate_bytes_accum options;
  arangodb::velocypack::Builder properties;

  {
    // optional float
    static const std::string fieldName("threshold");

    if (slice.hasKey(fieldName)) {
      auto field = slice.get(fieldName);

      if (!field.isNumber<float>()) {
        errorField = fieldName;

        return arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy();
      }

      options.threshold = field.getNumber<float>();

      if (options.threshold  < 0. || options.threshold  > 1.) {
        errorField = fieldName;

        return arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy();
      }
    }
  }

  properties.openObject();
  properties.add("type", arangodb::iresearch::toValuePair(POLICY_BYTES_ACCUM));
  properties.add("threshold", arangodb::velocypack::Value(options.threshold));
  properties.close();

  return arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy{
    irs::index_utils::consolidation_policy(options), std::move(properties)
  };
}

template<>
arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy createConsolidationPolicy<
    irs::index_utils::consolidate_docs_live
>(
    arangodb::velocypack::Slice const& slice,
    std::string& errorField
) {
  irs::index_utils::consolidate_docs_live options;
  arangodb::velocypack::Builder properties;

  {
    // optional float
    static const std::string fieldName("threshold");

    if (slice.hasKey(fieldName)) {
      auto field = slice.get(fieldName);

      if (!field.isNumber<float>()) {
        errorField = fieldName;

        return arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy();
      }

      options.threshold = field.getNumber<float>();

      if (options.threshold  < 0. || options.threshold  > 1.) {
        errorField = fieldName;

        return arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy();
      }
    }
  }

  properties.openObject();
  properties.add("type", arangodb::iresearch::toValuePair(POLICY_DOCS_LIVE));
  properties.add("threshold", arangodb::velocypack::Value(options.threshold));
  properties.close();

  return arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy{
    irs::index_utils::consolidation_policy(options), std::move(properties)
  };
}

template<>
arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy createConsolidationPolicy<
    irs::index_utils::consolidate_docs_fill
>(
    arangodb::velocypack::Slice const& slice,
    std::string& errorField
) {
  irs::index_utils::consolidate_docs_fill options;
  arangodb::velocypack::Builder properties;

  {
    // optional float
    static const std::string fieldName("threshold");

    if (slice.hasKey(fieldName)) {
      auto field = slice.get(fieldName);

      if (!field.isNumber<float>()) {
        errorField = fieldName;

        return arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy();
      }

      options.threshold = field.getNumber<float>();

      if (options.threshold  < 0. || options.threshold  > 1.) {
        errorField = fieldName;

        return arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy();
      }
    }
  }

  properties.openObject();
  properties.add("type", arangodb::iresearch::toValuePair(POLICY_DOCS_FILL));
  properties.add("threshold", arangodb::velocypack::Value(options.threshold));
  properties.close();

  return arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy{
    irs::index_utils::consolidation_policy(options), std::move(properties)
  };
}

template<>
arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy createConsolidationPolicy<
    irs::index_utils::consolidate_tier
>(
    arangodb::velocypack::Slice const& slice,
    std::string& errorField
) {
  irs::index_utils::consolidate_tier options;
  arangodb::velocypack::Builder properties;

  {
    // optional size_t
    static const std::string fieldName("lookahead");

    if (slice.hasKey(fieldName)) {
      auto field = slice.get(fieldName);

      if (!field.isNumber<size_t>()) {
        errorField = fieldName;

        return arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy();
      }

      options.lookahead = field.getNumber<size_t>();
    }
  }

  {
    // optional size_t
    static const std::string fieldName("segments_bytes_floor");

    if (slice.hasKey(fieldName)) {
      auto field = slice.get(fieldName);

      if (!field.isNumber<size_t>()) {
        errorField = fieldName;

        return arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy();
      }

      options.floor_segment_bytes = field.getNumber<size_t>();
    }
  }

  {
    // optional size_t
    static const std::string fieldName("segments_bytes_max");

    if (slice.hasKey(fieldName)) {
      auto field = slice.get(fieldName);

      if (!field.isNumber<size_t>()) {
        errorField = fieldName;

        return arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy();
      }

      options.max_segments_bytes = field.getNumber<size_t>();
    }
  }

  {
    // optional size_t
    static const std::string fieldName("segments_max");

    if (slice.hasKey(fieldName)) {
      auto field = slice.get(fieldName);

      if (!field.isNumber<size_t>()) {
        errorField = fieldName;

        return arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy();
      }

      options.max_segments = field.getNumber<size_t>();
    }
  }

  {
    // optional size_t
    static const std::string fieldName("segments_min");

    if (slice.hasKey(fieldName)) {
      auto field = slice.get(fieldName);

      if (!field.isNumber<size_t>()) {
        errorField = fieldName;

        return arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy();
      }

      options.min_segments = field.getNumber<size_t>();
    }
  }

  properties.openObject();
  properties.add("type", arangodb::iresearch::toValuePair(POLICY_TIER));
  properties.add("lookahead", arangodb::velocypack::Value(options.lookahead));
  properties.add("segments_bytes_floor", arangodb::velocypack::Value(options.floor_segment_bytes));
  properties.add("segments_bytes_max", arangodb::velocypack::Value(options.max_segments_bytes));
  properties.add("segments_max", arangodb::velocypack::Value(options.max_segments));
  properties.add("segments_min", arangodb::velocypack::Value(options.min_segments));
  properties.close();

  return arangodb::iresearch::IResearchViewMeta::ConsolidationPolicy{
    irs::index_utils::consolidation_policy(options), std::move(properties)
  };
}

NS_END

NS_BEGIN(arangodb)
NS_BEGIN(iresearch)
/*
static IResearchViewMeta::ConsolidationPolicy IResearchViewMeta::ConsolidationPolicy::make(
    arangodb::velocypack::Slice const& slice,
    std::string& errorField
) {
  if (!slice.isObject()) {
    return ConsolidationPolicy();
  }

  // required string enum
  static const std::string fieldName("type");

  if (!slice.hasKey(policyTypeField)) {
    errorField = policyTypeField;

    return ConsolidationPolicy();
  }

  auto field = slice.get(policyTypeField);

  if (!field.isString()) {
    errorField = policyTypeField;

    return ConsolidationPolicy();
  }

  auto policyType = field.copyString();

  if (POLICY_BYTES == policyType) {
    return createConsolidationPolicy<irs::index_utils::consolidate_bytes>(
      properties, errorField
    );
  } else if (POLICY_BYTES_ACCUM == policyType) {
    return createConsolidationPolicy<irs::index_utils::consolidate_bytes_accum>(
      properties, errorField
    );
  } else if (POLICY_DOCS_FILL == policyType) {
    return createConsolidationPolicy<irs::index_utils::consolidate_docs_fill>(
      properties, errorField
    );
  } else if (POLICY_DOCS_LIVE == policyType) {
    return createConsolidationPolicy<irs::index_utils::consolidate_docs_live>(
      properties, errorField
    );
  } else if (POLICY_TIER == policyType) {
    return createConsolidationPolicy<irs::index_utils::consolidate_tier>(
      properties, errorField
    );
  }

  errorField = policyTypeField;

  return ConsolidationPolicy();
}
*/
IResearchViewMeta::Mask::Mask(bool mask /*=false*/) noexcept
  : _cleanupIntervalStep(mask),
    _consolidationIntervalMsec(mask),
    _consolidationPolicy(mask),
    _locale(mask),
    _segmentCountMax(mask),
    _segmentDocsMax(mask),
    _segmentMemoryMax(mask) {
}

IResearchViewMeta::IResearchViewMeta()
  : _cleanupIntervalStep(10),
    _consolidationIntervalMsec(60 * 1000),
    _locale(std::locale::classic()),
    _segmentCountMax(0),
    _segmentDocsMax(0),
    _segmentMemoryMax(32*(size_t(1)<<20)) { // 32MB
  std::string errorField;

  _consolidationPolicy = createConsolidationPolicy<
    irs::index_utils::consolidate_bytes_accum
  >(
    arangodb::velocypack::Parser::fromJson(
      "{ \"type\": \"bytes_accum\", \"threshold\": 0.1 }"
    )->slice(),
    errorField
  );
  assert(_consolidationPolicy.policy()); // ensure above syntax is correct
}

IResearchViewMeta::IResearchViewMeta(IResearchViewMeta const& defaults)
  : _consolidationPolicy(DEFAULT()._consolidationPolicy) { // arbitrary value overwritten below
  *this = defaults;
}

IResearchViewMeta::IResearchViewMeta(IResearchViewMeta&& other) noexcept
  : _consolidationPolicy(DEFAULT()._consolidationPolicy) { // arbitrary value overwritten below
  *this = std::move(other);
}

IResearchViewMeta& IResearchViewMeta::operator=(IResearchViewMeta&& other) noexcept {
  if (this != &other) {
    _cleanupIntervalStep = std::move(other._cleanupIntervalStep);
    _consolidationIntervalMsec = std::move(other._consolidationIntervalMsec);
    _consolidationPolicy = std::move(other._consolidationPolicy);
    _locale = std::move(other._locale);
    _segmentCountMax = std::move(other._segmentCountMax);
    _segmentDocsMax = std::move(other._segmentDocsMax);
    _segmentMemoryMax = std::move(other._segmentMemoryMax);
  }

  return *this;
}

IResearchViewMeta& IResearchViewMeta::operator=(IResearchViewMeta const& other) {
  if (this != &other) {
    _cleanupIntervalStep = other._cleanupIntervalStep;
    _consolidationIntervalMsec = other._consolidationIntervalMsec;
    _consolidationPolicy = other._consolidationPolicy;
    _locale = other._locale;
    _segmentCountMax = other._segmentCountMax;
    _segmentDocsMax = other._segmentDocsMax;
    _segmentMemoryMax = other._segmentMemoryMax;
  }

  return *this;
}

bool IResearchViewMeta::operator==(IResearchViewMeta const& other) const noexcept {
  if (_cleanupIntervalStep != other._cleanupIntervalStep) {
    return false; // values do not match
  }

  if (_consolidationIntervalMsec != other._consolidationIntervalMsec) {
    return false; // values do not match
  }

  if (!_consolidationPolicy.properties().equals(other._consolidationPolicy.properties())) {
    return false; // values do not match
  }

  if (irs::locale_utils::language(_locale) != irs::locale_utils::language(other._locale)
      || irs::locale_utils::country(_locale) != irs::locale_utils::country(other._locale)
      || irs::locale_utils::encoding(_locale) != irs::locale_utils::encoding(other._locale)) {
    return false; // values do not match
  }

  if (_segmentCountMax != other._segmentCountMax) {
    return false; // values do not match
  }

  if (_segmentDocsMax != other._segmentDocsMax) {
    return false; // values do not match
  }

  if (_segmentMemoryMax != other._segmentMemoryMax) {
    return false; // values do not match
  }

  return true;
}

bool IResearchViewMeta::operator!=(
  IResearchViewMeta const& other
  ) const noexcept {
  return !(*this == other);
}

/*static*/ const IResearchViewMeta& IResearchViewMeta::DEFAULT() {
  static const IResearchViewMeta meta;

  return meta;
}

bool IResearchViewMeta::init(
  arangodb::velocypack::Slice const& slice,
  std::string& errorField,
  IResearchViewMeta const& defaults /*= DEFAULT()*/,
  Mask* mask /*= nullptr*/
) noexcept {
  if (!slice.isObject()) {
    return false;
  }

  Mask tmpMask;

  if (!mask) {
    mask = &tmpMask;
  }

  {
    // optional size_t
    static const std::string fieldName("cleanupIntervalStep");

    mask->_cleanupIntervalStep = slice.hasKey(fieldName);

    if (!mask->_cleanupIntervalStep) {
      _cleanupIntervalStep = defaults._cleanupIntervalStep;
    } else {
      auto field = slice.get(fieldName);

      if (!getNumber(_cleanupIntervalStep, field)) {
        errorField = fieldName;

        return false;
      }
    }
  }

  {
    // optional size_t
    static const std::string fieldName("consolidationIntervalMsec");

    mask->_consolidationIntervalMsec = slice.hasKey(fieldName);

    if (!mask->_consolidationIntervalMsec) {
      _consolidationIntervalMsec = defaults._consolidationIntervalMsec;
    } else {
      auto field = slice.get(fieldName);

      if (!getNumber(_consolidationIntervalMsec, field)) {
        errorField = fieldName;

        return false;
      }
    }
  }

  {
    // optional object
    static const std::string fieldName("consolidationPolicy");
    std::string errorSubField;

    mask->_consolidationPolicy = slice.hasKey(fieldName);

    if (!mask->_consolidationPolicy) {
      _consolidationPolicy = defaults._consolidationPolicy;
    } else {
      auto field = slice.get(fieldName);

      if (!field.isObject()) {
        errorField = fieldName;

        return false;
      }

      // required string enum
      static const std::string typeFieldName("type");

      if (!field.hasKey(typeFieldName)) {
        errorField = fieldName + "=>" + typeFieldName;

        return false;
      }

      auto typeField = field.get(typeFieldName);

      if (!typeField.isString()) {
        errorField = fieldName + "=>" + typeFieldName;

        return false;
      }

      auto type = typeField.copyString();

      if (POLICY_BYTES == type) {
        _consolidationPolicy = createConsolidationPolicy<
          irs::index_utils::consolidate_bytes
        >(field, errorSubField);
      } else if (POLICY_BYTES_ACCUM == type) {
        _consolidationPolicy = createConsolidationPolicy<
          irs::index_utils::consolidate_bytes_accum
        >(field, errorSubField);
      } else if (POLICY_DOCS_FILL == type) {
        _consolidationPolicy = createConsolidationPolicy<
          irs::index_utils::consolidate_docs_fill
        >(field, errorSubField);
      } else if (POLICY_DOCS_LIVE == type) {
        _consolidationPolicy = createConsolidationPolicy<
          irs::index_utils::consolidate_docs_live
        >(field, errorSubField);
      } else if (POLICY_TIER == type) {
        _consolidationPolicy = createConsolidationPolicy<
          irs::index_utils::consolidate_tier
        >(field, errorSubField);
      } else {
        errorField = fieldName + "=>" + typeFieldName;

        return false;
      }

      if (!_consolidationPolicy.policy()) {
        if (errorSubField.empty()) {
          errorField = fieldName;
        } else {
          errorField = fieldName + "=>" + errorSubField;
        }

        return false;
      }
    }
  }
/* FIXME TODO temporarily disable, eventually used for ordering internal data structures
  {
    // optional locale name
    static const std::string fieldName("locale");

    mask->_locale = slice.hasKey(fieldName);

    if (!mask->_locale) {
      _locale = defaults._locale;
    } else {
      auto field = slice.get(fieldName);

      if (!field.isString()) {
        errorField = fieldName;

        return false;
      }

      auto locale = field.copyString();

      try {
        // use UTF-8 encoding since that is what JSON objects use
        _locale = std::locale::classic().name() == locale
          ? std::locale::classic() : irs::locale_utils::locale(locale);
      } catch(...) {
        errorField = fieldName;

        return false;
      }
    }
  }
*/

  {
    // optional size_t
    static const std::string fieldName("segmentCountMax");

    mask->_segmentCountMax = slice.hasKey(fieldName);

    if (!mask->_segmentCountMax) {
      _segmentCountMax = defaults._segmentCountMax;
    } else {
      auto field = slice.get(fieldName);

      if (!getNumber(_segmentCountMax, field)) {
        errorField = fieldName;

        return false;
      }
    }
  }

  {
    // optional size_t
    static const std::string fieldName("segmentDocsMax");

    mask->_segmentDocsMax = slice.hasKey(fieldName);

    if (!mask->_segmentDocsMax) {
      _segmentDocsMax = defaults._segmentDocsMax;
    } else {
      auto field = slice.get(fieldName);

      if (!getNumber(_segmentDocsMax, field)) {
        errorField = fieldName;

        return false;
      }
    }
  }

  {
    // optional size_t
    static const std::string fieldName("segmentMemoryMax");

    mask->_segmentMemoryMax = slice.hasKey(fieldName);

    if (!mask->_segmentMemoryMax) {
      _segmentMemoryMax = defaults._segmentMemoryMax;
    } else {
      auto field = slice.get(fieldName);

      if (!getNumber(_segmentMemoryMax, field)) {
        errorField = fieldName;

        return false;
      }
    }
  }

  return true;
}

bool IResearchViewMeta::json(
  arangodb::velocypack::Builder& builder,
  IResearchViewMeta const* ignoreEqual /*= nullptr*/,
  Mask const* mask /*= nullptr*/
) const {
  if (!builder.isOpenObject()) {
    return false;
  }

  if ((!ignoreEqual || _cleanupIntervalStep != ignoreEqual->_cleanupIntervalStep) && (!mask || mask->_cleanupIntervalStep)) {
    builder.add("cleanupIntervalStep", arangodb::velocypack::Value(_cleanupIntervalStep));
  }

  if ((!ignoreEqual || _consolidationIntervalMsec != ignoreEqual->_consolidationIntervalMsec) && (!mask || mask->_consolidationIntervalMsec)) {
    builder.add("consolidationIntervalMsec", arangodb::velocypack::Value(_consolidationIntervalMsec));
  }

  if ((!ignoreEqual || !_consolidationPolicy.properties().equals(ignoreEqual->_consolidationPolicy.properties())) && (!mask || mask->_consolidationPolicy)) {
    builder.add("consolidationPolicy", _consolidationPolicy.properties());
  }
/* FIXME TODO temporarily disable, eventually used for ordering internal data structures
  if ((!ignoreEqual || _locale != ignoreEqual->_locale) && (!mask || mask->_locale)) {
    builder.add("locale", arangodb::velocypack::Value(irs::locale_utils::name(_locale)));
  }
*/

  if ((!ignoreEqual || _segmentCountMax != ignoreEqual->_segmentCountMax) && (!mask || mask->_segmentCountMax)) {
    builder.add("segmentCountMax", arangodb::velocypack::Value(_segmentCountMax));
  }

  if ((!ignoreEqual || _segmentDocsMax != ignoreEqual->_segmentDocsMax) && (!mask || mask->_segmentDocsMax)) {
    builder.add("segmentDocsMax", arangodb::velocypack::Value(_segmentDocsMax));
  }

  if ((!ignoreEqual || _segmentMemoryMax != ignoreEqual->_segmentMemoryMax) && (!mask || mask->_segmentMemoryMax)) {
    builder.add("segmentMemoryMax", arangodb::velocypack::Value(_segmentMemoryMax));
  }

  return true;
}

bool IResearchViewMeta::json(
  arangodb::velocypack::ObjectBuilder const& builder,
  IResearchViewMeta const* ignoreEqual /*= nullptr*/,
  Mask const* mask /*= nullptr*/
) const {
  return builder.builder && json(*(builder.builder), ignoreEqual, mask);
}

size_t IResearchViewMeta::memory() const {
  auto size = sizeof(IResearchViewMeta);

  return size;
}

IResearchViewMetaState::Mask::Mask(bool mask /*=false*/) noexcept
  : _collections(mask) {
}

IResearchViewMetaState::IResearchViewMetaState() {
}

IResearchViewMetaState::IResearchViewMetaState(
    IResearchViewMetaState const& defaults
) {
  *this = defaults;
}

IResearchViewMetaState::IResearchViewMetaState(
    IResearchViewMetaState&& other
) noexcept {
  *this = std::move(other);
}

IResearchViewMetaState& IResearchViewMetaState::operator=(
    IResearchViewMetaState&& other
) noexcept {
  if (this != &other) {
    _collections = std::move(other._collections);
  }

  return *this;
}

IResearchViewMetaState& IResearchViewMetaState::operator=(
    IResearchViewMetaState const& other
) {
  if (this != &other) {
    _collections = other._collections;
  }

  return *this;
}

bool IResearchViewMetaState::operator==(
    IResearchViewMetaState const& other
) const noexcept {
  if (_collections != other._collections) {
    return false; // values do not match
  }

  return true;
}

bool IResearchViewMetaState::operator!=(
  IResearchViewMetaState const& other
  ) const noexcept {
  return !(*this == other);
}

/*static*/ const IResearchViewMetaState& IResearchViewMetaState::DEFAULT() {
  static const IResearchViewMetaState meta;

  return meta;
}

bool IResearchViewMetaState::init(
  arangodb::velocypack::Slice const& slice,
  std::string& errorField,
  IResearchViewMetaState const& defaults /*= DEFAULT()*/,
  Mask* mask /*= nullptr*/
) {
  if (!slice.isObject()) {
    errorField = "not an object";
    return false;
  }

  Mask tmpMask;

  if (!mask) {
    mask = &tmpMask;
  }

  {
    // optional uint64 list
    static const std::string fieldName("collections");

    mask->_collections = slice.hasKey(fieldName);

    if (!mask->_collections) {
      _collections = defaults._collections;
    } else {
      auto field = slice.get(fieldName);

      if (!field.isArray()) {
        errorField = fieldName;

        return false;
      }

      _collections.clear(); // reset to match read values exactly

      for (arangodb::velocypack::ArrayIterator itr(field); itr.valid(); ++itr) {
        decltype(_collections)::key_type value;

        if (!getNumber(value, itr.value())) { // [ <collectionId 1> ... <collectionId N> ]
          errorField = fieldName + "=>[" + arangodb::basics::StringUtils::itoa(itr.index()) + "]";

          return false;
        }

        _collections.emplace(value);
      }
    }
  }

  return true;
}

bool IResearchViewMetaState::json(
  arangodb::velocypack::Builder& builder,
  IResearchViewMetaState const* ignoreEqual /*= nullptr*/,
  Mask const* mask /*= nullptr*/
) const {
  if (!builder.isOpenObject()) {
    return false;
  }

  if ((!ignoreEqual || _collections != ignoreEqual->_collections) && (!mask || mask->_collections)) {
    arangodb::velocypack::Builder subBuilder;

    {
      arangodb::velocypack::ArrayBuilder subBuilderWrapper(&subBuilder);

      for (auto& cid: _collections) {
        subBuilderWrapper->add(arangodb::velocypack::Value(cid));
      }
    }

    builder.add("collections", subBuilder.slice());
  }

  return true;
}

bool IResearchViewMetaState::json(
  arangodb::velocypack::ObjectBuilder const& builder,
  IResearchViewMetaState const* ignoreEqual /*= nullptr*/,
  Mask const* mask /*= nullptr*/
) const {
  return builder.builder && json(*(builder.builder), ignoreEqual, mask);
}

size_t IResearchViewMetaState::memory() const {
  auto size = sizeof(IResearchViewMetaState);

  size += sizeof(TRI_voc_cid_t) * _collections.size();

  return size;
}

NS_END // iresearch
NS_END // arangodb

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------