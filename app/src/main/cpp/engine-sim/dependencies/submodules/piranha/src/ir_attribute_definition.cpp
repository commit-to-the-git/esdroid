#include "../include/ir_attribute_definition.h"

#include "../include/ir_value.h"
#include "../include/ir_compilation_unit.h"
#include "../include/ir_context_tree.h"
#include "../include/ir_node_definition.h"
#include "../include/ir_node.h"
#include "../include/ir_attribute.h"
#include "../include/ir_attribute_list.h"
#include "../include/ir_value_constant.h"
#include "../include/compilation_error.h"
#include "../include/node_output.h"
#include "../include/language_rules.h"
#include "../include/standard_allocator.h"
#include "../include/memory_tracker.h"

piranha::IrAttributeDefinition::IrAttributeDefinition(
    const IrTokenInfo_string &directionToken, const IrTokenInfo_string &name, Direction dir)
{
    m_name = name;
    registerToken(&m_name);

    m_directionToken = directionToken;
    registerToken(&m_directionToken);

    m_direction = dir;
    setVisibility(IrVisibility::Public);

    m_typeDefinition = nullptr;
}

piranha::IrAttributeDefinition::IrAttributeDefinition(const IrTokenInfo_string &name) {
    m_name = name;
    registerToken(&m_name);

    m_direction = Direction::Output;
    setVisibility(IrVisibility::Public);

    m_typeDefinition = nullptr;
}

piranha::IrAttributeDefinition::~IrAttributeDefinition() {
    /* void */
}

void piranha::IrAttributeDefinition::setDefaultValue(IrValue *value) {
    m_defaultValue = value;
    registerComponent(m_defaultValue);
}

void piranha::IrAttributeDefinition::setAliasToken(const IrTokenInfo_string &aliasToken) {
    m_aliasToken = aliasToken;
    registerToken(&m_aliasToken);
}

piranha::IrParserStructure *piranha::IrAttributeDefinition::getImmediateReference(
    const IrReferenceQuery &query, IrReferenceInfo *output)
{
    IR_RESET(query);

    // if a type definition is present then this chain of references must have a fixed type
    if (m_typeDefinition != nullptr) {
        IR_INFO_OUT(fixedType, getTypeDefinition());
    }
    else if (isInput()) {
        IR_INFO_OUT(staticType, false);
    }

    // first check the input context for the reference
    if (!IR_EMPTY_CONTEXT()) {
        IrParserStructure *reference = query.inputContext->resolveDefinition(this);
        if (reference != nullptr) {
            IrNode **expansion = m_expansions.lookup(query.inputContext);
            if (*expansion != nullptr) return *expansion;

            IR_INFO_OUT(newContext, query.inputContext->getParent());

            // this flag must be set to notify that this chain of resolutions actually
            // touched the important main context the only context for which errors
            // are being logged
            IR_INFO_OUT(touchedMainContext, query.inputContext->isMainContext());

            return reference;
        }
    }

    // an attribute definition will by default point to its definition ie default value
    if (m_defaultValue == nullptr && isInput()) {
        if (IR_EMPTY_CONTEXT() || query.inputContext->getContext()->isInterface()) {
            IrNode **expansion = m_expansions.lookup(query.inputContext);

            if (*expansion == nullptr) {
                IR_DEAD_END();
                return nullptr;
            }

            return *expansion;
        }
        else {
            // this is the result of an unconnected input that has no default
            IR_FAIL();
            return nullptr;
        }
    }
    else if (m_defaultValue == nullptr && m_direction == Direction::Output) {
        if (m_typeDefinition == nullptr) {
            IR_FAIL();
            return nullptr;
        }
        else {
            IrNode **node = m_expansions.lookup(query.inputContext);
            if (node == nullptr) {
                IR_FAIL();
                return nullptr;
            }
            return *node;
        }
    }

    // look up the expansion for the default case and return it if it exists
    IrNode **expansion = m_expansions.lookup(query.inputContext);
    if (expansion == nullptr) {
        IR_FAIL();
        return nullptr;
    }

    if (*expansion != nullptr) return *expansion;

    return m_defaultValue;
}

piranha::IrNodeDefinition *piranha::IrAttributeDefinition::getTypeDefinition() const {
    return (m_typeDefinition != nullptr)
        ? m_typeDefinition->getAliasType()
        : nullptr;
}

const piranha::ChannelType *piranha::IrAttributeDefinition::getImmediateChannelType() {
    if (m_typeDefinition != nullptr) {
        if (!m_typeDefinition->isBuiltin()) return nullptr;
        std::string builtinType = m_typeDefinition->getBuiltinName();

        const ChannelType *wholeType = m_rules->resolveChannelType(builtinType);
        return wholeType;
    }
    else return nullptr;
}

void piranha::IrAttributeDefinition::_expand(IrContextTree *context) {
    if (m_typeDefinition == nullptr) return;

    if (m_direction == Direction::Output && m_defaultValue == nullptr) {
        IrNode *expansion = TRACK(new IrNode);
        expansion->setLogicalParent(this);
        expansion->setScopeParent(this);
        expansion->setInterface(true);
        expansion->setDefinition(getTypeDefinition());
        expansion->setRules(m_rules);
        expansion->expand(context);

        *m_expansions.newValue(context) = expansion;
    }
    else {
        IrReferenceInfo immediateInfo;
        IrReferenceQuery immediateQuery;
        immediateQuery.inputContext = context;
        immediateQuery.recordErrors = false;
        IrParserStructure *immediateReference =
            getImmediateReference(immediateQuery, &immediateInfo);

        if (immediateInfo.failed) return;
        else if (immediateInfo.reachedDeadEnd) {
            IrNode *expansion = TRACK(new IrNode);
            expansion->setLogicalParent(this);
            expansion->setScopeParent(this);
            expansion->setInterface(true);
            expansion->setBuild(false);
            expansion->setDefinition(getTypeDefinition());
            expansion->setRules(m_rules);
            expansion->expand(context);

            *m_expansions.newValue(context) = expansion;

            return;
        }

        // make sure the entire reference chain is expanded
        immediateReference->expandChain(immediateInfo.newContext);

        IrReferenceInfo referenceInfo;
        IrReferenceQuery referenceQuery;
        referenceQuery.inputContext = immediateInfo.newContext;
        referenceQuery.recordErrors = false;
        IrParserStructure *reference =
            immediateReference->getReference(referenceQuery, &referenceInfo);

        if (referenceInfo.failed) return;
        if (referenceInfo.reachedDeadEnd) {
            // a dead end is okay as long as the fixed type is known
            if (!referenceInfo.isFixedType()) return;
            else reference = referenceInfo.fixedType;
        }

        IrNodeDefinition *fixedTypeDefinition = (referenceInfo.isFixedType())
            ? referenceInfo.fixedType
            : nullptr;
        // important fixed type information from the immediate reference cannot be
        // used because it will always get forced to the type of this attribute definition
        // thereby preventing automatic conversion from taking place

        const ChannelType *referenceType = (fixedTypeDefinition != nullptr)
            ? fixedTypeDefinition->getChannelType()
            : reference->getImmediateChannelType();
        const ChannelType *expectedType = getTypeDefinition()->getChannelType();

        if (referenceType == nullptr || expectedType == nullptr) {
            IrNode *asNode = reference->getAsNode();
            IrNodeDefinition *typeDefinition = (asNode != nullptr)
                ? asNode->getDefinition()
                : fixedTypeDefinition;

            if (typeDefinition != nullptr && typeDefinition != getTypeDefinition()) {
                // this error was already detected in an earlier step
                return;
            }
        }
        else if (referenceType == expectedType) return; // no expansion/conversion needed

        // stop if language rules are not specified
        // note - this should only really be used in unit testing
        if (m_rules == nullptr) return;

        std::string builtinType =
            m_rules->resolveConversionBuiltinType(referenceType, expectedType);

        if (builtinType.empty()) return; // incompatible types

        IrCompilationUnit *parentUnit = (context->isEmpty())
            ? getParentUnit()
            : context->getContext()->getParentUnit();

        int count = 0;
        IrNodeDefinition *nodeDefinition =
            parentUnit->resolveBuiltinNodeDefinition(builtinType, &count);

        // create expanion structure
        IrInternalReference *internalReference =
            TRACK(new IrInternalReference(reference, referenceInfo.newContext));

        IrAttribute *input = TRACK(new IrAttribute);
        input->setValue(internalReference);

        IrAttributeList *attributeList = TRACK(new IrAttributeList);
        attributeList->addAttribute(input);

        IrNode *expansion = TRACK(new IrNode);
        expansion->setAttributes(attributeList);
        expansion->setLogicalParent(this);
        expansion->setScopeParent(this);
        expansion->setDefinition(nodeDefinition);
        expansion->setRules(m_rules);
        expansion->expand(context);
        expansion->resolveDefinitions();

        *m_expansions.newValue(context) = expansion;
    }
}

void piranha::IrAttributeDefinition::_checkTypes(IrContextTree *context) {
    // skips this step completely if language rules are not specified
    // note - this should only really be used in unit testing
    if (m_rules == nullptr) return;

    if (getTypeDefinition() != nullptr && m_defaultValue != nullptr) {
        IrReferenceInfo info;
        IrReferenceQuery query;
        query.inputContext = context;
        query.recordErrors = false;
        IrParserStructure *defaultReference = m_defaultValue->getReference(query, &info);

        if (info.failed) return;
        if (info.reachedDeadEnd) {
            // a dead end is okay as long as the fixed type is known
            if (!info.isFixedType()) return;
            else defaultReference = info.fixedType;
        }

        if (!info.touchedMainContext) {
            if (!context->isEmpty()) return;
            if (!context->isMainContext()) return;
        }

        // if the incoming type is fixed then only the null context would interest us
        if (info.isStaticType() && !context->isEmpty()) return;

        IrNode *refAsNode = defaultReference->getAsNode();
        if (refAsNode != nullptr) {
            IrNodeDefinition *definition = refAsNode->getDefinition()->getAliasType();
            if (definition == nullptr) return; // a syntax error must have already occurred
            if (definition == getTypeDefinition()) return; // type is confirmed to be correct
        }

        const ChannelType *type = info.isFixedType()
            ? info.fixedType->getChannelType()
            : defaultReference->getImmediateChannelType();
        const ChannelType *expectedType = getTypeDefinition()->getChannelType();

        if (type == expectedType && expectedType != nullptr) return; // no conversion necessary
        if (m_rules->checkConversion(type, expectedType)) return; // conversion is valid

        // conversion is invalid
        IrCompilationUnit *unit = getParentUnit();

        // errors for inputs/outputs differ only in code but are fundamentally the same
        if (m_direction == Direction::Input ||
            m_direction == Direction::Modify ||
            m_direction == Direction::Output)
        {
            unit->addCompilationError(TRACK(new CompilationError(*m_defaultValue->getSummaryToken(),
                ErrorCode::IncompatibleDefaultType, context)));
        }
        else if (m_direction == Direction::Output) {
            unit->addCompilationError(TRACK(new CompilationError(*m_defaultValue->getSummaryToken(),
                ErrorCode::IncompatibleOutputDefinitionType, context)));
        }
    }
}

bool piranha::IrAttributeDefinition::isInput() const {
    return
        m_direction == Direction::Input ||
        m_direction == Direction::Modify ||
        m_direction == Direction::Toggle;
}

bool piranha::IrAttributeDefinition::isThis() const {
    return getName() == "this";
}

void piranha::IrAttributeDefinition::_resolveDefinitions() {
    IrNodeDefinition *definition = nullptr;
    IrCompilationUnit *unit = getParentUnit();

    const IrTokenInfo_string &libraryToken = m_typeInfo.data[0];
    const IrTokenInfo_string &typeToken = m_typeInfo.data[1];

    // no action is needed if a type wasnt specified
    if (!typeToken.specified) return;

    const std::string &type = typeToken.data;

    int definitionCount = 0;
    if (libraryToken.specified) {
        const std::string &library = libraryToken.data;

        if (!library.empty()) {
            definition = unit->resolveNodeDefinition(type, &definitionCount, library);
        }
        else {
            // adding an empty library name means that the local scope must strictly be used
            definition = unit->resolveLocalNodeDefinition(type, &definitionCount);
        }
    }
    else definition = unit->resolveNodeDefinition(type, &definitionCount, "");

    if (definitionCount > 0) {
        // todo log a warning when a node type is ambiguous
    }

    if (definition == nullptr) {
        unit->addCompilationError(TRACK(new CompilationError(typeToken,
            ErrorCode::UndefinedNodeType)));
        m_typeDefinition = nullptr;
    }
    else {
        m_typeDefinition = definition;
    }
}

piranha::Node *piranha::IrAttributeDefinition::_generateNode(
    IrContextTree *context, NodeProgram *program, NodeContainer *container)
{
    if (
        m_typeDefinition != nullptr &&
        m_direction == Direction::Output &&
        m_defaultValue == nullptr)
    {
        // this must be an interface
        Node *interfaceNode = context
            ->getContext()
            ->_generateNode(context->getParent(), program, container)
            ->getOutput(getName())
            ->generateInterface(program->getNodeAllocator());

        if (interfaceNode != nullptr) {
            program->addNode(interfaceNode);
        }

        return interfaceNode;
    }

    return IrParserStructure::_generateNode(context, program, container);
}

piranha::NodeOutput *piranha::IrAttributeDefinition::_generateNodeOutput(
    IrContextTree *context, NodeProgram *program, NodeContainer *container)
{
    if (
        m_typeDefinition != nullptr &&
        m_direction == Direction::Output &&
        m_defaultValue == nullptr)
    {
        // this must be an interface
        return context
            ->getContext()
            ->generateNode(context->getParent(), program, container)
            ->getOutput(getName());
    }

    return IrParserStructure::_generateNodeOutput(context, program, container);
}
