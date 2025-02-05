#pragma once

#include <ydb/library/yql/ast/yql_expr.h>
#include <ydb/library/yql/dq/expr_nodes/dq_expr_nodes.h>
#include <ydb/library/yql/dq/proto/dq_tasks.pb.h>

#include <util/generic/guid.h>

namespace NYql::NDq {

struct TDqStageSettings {
    static constexpr TStringBuf LogicalIdSettingName = "_logical_id";
    static constexpr TStringBuf IdSettingName = "_id";
    static constexpr TStringBuf SinglePartitionSettingName = "_single_partition";
    static constexpr TStringBuf WideChannelsSettingName = "_wide_channels";

    ui64 LogicalId = 0;
    TString Id;
    bool SinglePartition = false;

    bool WideChannels = false;
    const TStructExprType* OutputNarrowType = nullptr;

    TDqStageSettings& SetSinglePartition(bool value = true) { SinglePartition = value; return *this; }
    TDqStageSettings& SetWideChannels(const TStructExprType& narrowType) { WideChannels = true; OutputNarrowType = &narrowType; return *this; }

    static TDqStageSettings New(const NNodes::TDqStageBase& node);

    static TDqStageSettings New() {
        TDqStageSettings s;
        s.Id = CreateGuidAsString();
        return s;
    }

    static TDqStageSettings Parse(const NNodes::TDqStageBase& node);
    NNodes::TCoNameValueTupleList BuildNode(TExprContext& ctx, TPositionHandle pos) const;
};

NNodes::TCoAtom BuildAtom(TStringBuf value, TPositionHandle pos, TExprContext& ctx);
NNodes::TCoAtomList BuildAtomList(TStringBuf value, TPositionHandle pos, TExprContext& ctx);
NNodes::TCoLambda BuildIdentityLambda(TPositionHandle pos, TExprContext& ctx);

bool EnsureDqUnion(const NNodes::TExprBase& node, TExprContext& ctx);

const TNodeSet& GetConsumers(const NNodes::TExprBase& node, const TParentsMap& parentsMap);
const TNodeMultiSet& GetConsumers(const NNodes::TExprBase& node, const TParentsMultiMap& parentsMap);

ui32 GetConsumersCount(const NNodes::TExprBase& node, const TParentsMap& parentsMap);
bool IsSingleConsumer(const NNodes::TExprBase& node, const TParentsMap& parentsMap);

bool IsSingleConsumerConnection(const NNodes::TDqConnection& node, const TParentsMap& parentsMap, bool allowStageMultiUsage = true);

ui32 GetStageOutputsCount(const NNodes::TDqStageBase& stage);

void FindDqConnections(const NNodes::TExprBase& node, TVector<NNodes::TDqConnection>& connections, bool& isPure);
bool IsDqPureExpr(const NNodes::TExprBase& node, bool isPrecomputePure = true);
bool IsDqSelfContainedExpr(const NNodes::TExprBase& node);
bool IsDqDependsOnStage(const NNodes::TExprBase& node, const NNodes::TDqStageBase& stage);

bool CanPushDqExpr(const NNodes::TExprBase& expr, const NNodes::TDqStageBase& stage);
bool CanPushDqExpr(const NNodes::TExprBase& expr, const NNodes::TDqConnection& connection);

} // namespace NYql::NDq
