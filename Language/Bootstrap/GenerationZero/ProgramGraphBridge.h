#ifndef RANGE_PROGRAM_GRAPH_BRIDGE_H
#define RANGE_PROGRAM_GRAPH_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

enum RangeOperationKind {
    RangeOperationConstant = 1,
    RangeOperationAllocate = 2,
    RangeOperationAddress = 3,
    RangeOperationStore = 4,
    RangeOperationLoad = 5,
    RangeOperationCall = 6,
    RangeOperationDestroy = 7,
    RangeOperationReturn = 8,
    RangeOperationBranch = 9,
    RangeOperationOutput = 10,
    RangeOperationCompare = 11,
    RangeOperationLogical = 12
};

struct RangeIntColumn {
    int32_t *values;
    size_t count;
    size_t capacity;
};

struct RangeProgramGraphBridge {
    struct RangeIntColumn functionSourceIDs;
    struct RangeIntColumn functionSyntaxIDs;
    struct RangeIntColumn functionEntryBlockIDs;
    struct RangeIntColumn functionReturnTypeIDs;
    struct RangeIntColumn functionExternCodes;
    struct RangeIntColumn functionNameByteStarts;
    struct RangeIntColumn functionNameByteCounts;
    struct RangeIntColumn functionNameBytes;
    struct RangeIntColumn functionParameterFunctionIDs;
    struct RangeIntColumn functionParameterOrdinals;
    struct RangeIntColumn functionParameterTypeIDs;
    struct RangeIntColumn functionParameterValueIDs;
    struct RangeIntColumn typeDeclarationRows;
    struct RangeIntColumn typeRepresentationCodes;
    struct RangeIntColumn typeWidths;
    struct RangeIntColumn typeAlignments;
    struct RangeIntColumn blockFunctionIDs;
    struct RangeIntColumn blockSyntaxIDs;
    struct RangeIntColumn blockTerminatedCodes;
    struct RangeIntColumn operationBlockIDs;
    struct RangeIntColumn operationKindCodes;
    struct RangeIntColumn operationSourceIDs;
    struct RangeIntColumn operationSyntaxIDs;
    struct RangeIntColumn operationDeclarationRows;
    struct RangeIntColumn operationTargetFunctionIDs;
    struct RangeIntColumn operationImmediates;
    struct RangeIntColumn operationWidths;
    struct RangeIntColumn valueProducerOperationIDs;
    struct RangeIntColumn valueProducerOrdinals;
    struct RangeIntColumn valueTypeIDs;
    struct RangeIntColumn valueRepresentationCodes;
    struct RangeIntColumn valueWidths;
    struct RangeIntColumn valueOwnershipCodes;
    struct RangeIntColumn operandOperationIDs;
    struct RangeIntColumn operandOrdinals;
    struct RangeIntColumn operandValueIDs;
    struct RangeIntColumn effectSourceOperationIDs;
    struct RangeIntColumn effectTargetOperationIDs;
    struct RangeIntColumn controlSourceBlockIDs;
    struct RangeIntColumn controlTargetBlockIDs;
    struct RangeIntColumn controlPredicateValueIDs;
    struct RangeIntColumn controlExpectedValues;
    struct RangeIntColumn controlOrdinals;
    struct RangeIntColumn blockArgumentBlockIDs;
    struct RangeIntColumn blockArgumentOrdinals;
    struct RangeIntColumn blockArgumentValueIDs;
    struct RangeIntColumn blockArgumentTypeIDs;
};

const char *rangeProgramGraphBridgeSchema(void);

#endif
