#pragma once

#include <ydb/library/yql/dq/common/dq_common.h>
#include <ydb/library/yql/dq/common/dq_value.h>
#include <ydb/library/yql/dq/proto/dq_transport.pb.h>

#include <ydb/library/yql/ast/yql_expr.h>
#include <ydb/library/yql/minikql/mkql_function_registry.h>
#include <ydb/library/yql/minikql/computation/mkql_computation_node.h>
#include <ydb/library/yql/minikql/computation/mkql_computation_node_pack.h>


namespace NYql::NDq {

class TDqDataSerializer : private TNonCopyable {
public:
    TDqDataSerializer(const NKikimr::NMiniKQL::TTypeEnvironment& typeEnv,
        const NKikimr::NMiniKQL::THolderFactory& holderFactory, NDqProto::EDataTransportVersion transportVersion)
        : TypeEnv(typeEnv)
        , HolderFactory(holderFactory)
        , TransportVersion(transportVersion) {}

    NDqProto::EDataTransportVersion GetTransportVersion() const;

    NDqProto::TData Serialize(const NUdf::TUnboxedValue& value, const NKikimr::NMiniKQL::TType* itemType) const;

    template <class TForwardIterator>
    NDqProto::TData Serialize(TForwardIterator first, TForwardIterator last, const NKikimr::NMiniKQL::TType* itemType) const {
        const auto listType = NKikimr::NMiniKQL::TListType::Create(const_cast<NKikimr::NMiniKQL::TType*>(itemType), TypeEnv);
        if (TransportVersion == NDqProto::DATA_TRANSPORT_VERSION_UNSPECIFIED ||
            TransportVersion == NDqProto::DATA_TRANSPORT_UV_PICKLE_1_0)
        {
            NKikimr::NMiniKQL::TValuePackerTransport<false> packer(listType);
            return SerializeBatch(packer, first, last);
        }
        
        if (TransportVersion == NDqProto::DATA_TRANSPORT_UV_FAST_PICKLE_1_0) {
            NKikimr::NMiniKQL::TValuePackerTransport<true> packer(listType);
            return SerializeBatch(packer, first, last);
        }
        YQL_ENSURE(false, "Unsupported TransportVersion");
    }

    void Deserialize(const NDqProto::TData& data, const NKikimr::NMiniKQL::TType* itemType,
        NKikimr::NMiniKQL::TUnboxedValueVector& buffer) const;
    void Deserialize(const NDqProto::TData& data, const NKikimr::NMiniKQL::TType* itemType, NUdf::TUnboxedValue& value) const;

    struct TEstimateSizeSettings {
        bool WithHeaders;
        bool DiscardUnsupportedTypes;

        TEstimateSizeSettings() {
            WithHeaders = true;
            DiscardUnsupportedTypes = false;
        }
    };

    static ui64 EstimateSize(const NUdf::TUnboxedValue& value, const NKikimr::NMiniKQL::TType* type, bool* fixed = nullptr, TEstimateSizeSettings = {});

    static void DeserializeParam(const NDqProto::TData& data, const NKikimr::NMiniKQL::TType* type,
        const NKikimr::NMiniKQL::THolderFactory& holderFactory, NKikimr::NUdf::TUnboxedValue& value);

    static NDqProto::TData SerializeParamValue(const NKikimr::NMiniKQL::TType* type, const NUdf::TUnboxedValuePod& value);

public:
    const NKikimr::NMiniKQL::TTypeEnvironment& TypeEnv;
    const NKikimr::NMiniKQL::THolderFactory& HolderFactory;
    const NDqProto::EDataTransportVersion TransportVersion;
private:
    template <class TForwardIterator, class TPacker>
    NDqProto::TData SerializeBatch(TPacker& packer, TForwardIterator first, TForwardIterator last) const {
        size_t count = 0;
        while (first != last) {
            packer.AddItem(*first);
            ++first;
            ++count;
        }
        const auto& packed = packer.Finish();
        NDqProto::TData data;
        data.SetTransportVersion(TransportVersion);
        data.MutableRaw()->reserve(packed.Size());
        packed.CopyTo(*data.MutableRaw());
        data.SetRows(count);
        return data;
    }

};

} // namespace NYql::NDq
