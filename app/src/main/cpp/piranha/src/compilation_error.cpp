#include "../include/compilation_error.h"

#include "../include/ir_context_tree.h"
#include "../include/ir_parser_structure.h"

piranha::CompilationError::CompilationError(const IrTokenInfo &location, 
            const ErrorCode_struct &code, IrContextTree *instantiation) 
{
    m_errorLocation = location;
    m_code = code;
    m_instantiation = instantiation;
}

piranha::CompilationError::~CompilationError() {
    /* void */
}

bool piranha::CompilationError::isInstantiationError() const {
    return (m_instantiation != nullptr && !m_instantiation->isEmpty());
}

// error definitions --------------------------------------

// helper macro
#define ERR(tag) const piranha::ErrorCode_struct piranha::ErrorCode::tag

// io - io errors
ERR(FileOpenFailed) =           { "IO", "0010", "Could not open file" };

// s - scanning errors
ERR(UnidentifiedToken) =        { "S", "0010", "Unidentified token" };

// p - parsing errors
ERR(UnexpectedToken) =          { "P", "0010", "Unexpected token" };

// e - expansion errors
ERR(InvalidOperandTypes) =      { "E", "0010", "Invalid operand types" };
ERR(CircularDefinition) =       { "E", "0020", "Circular definition detected" };

// r - resolution errors
ERR(UndefinedNodeType) =        { "R", "0010", "Undefined node type" };
ERR(ArgumentPositionOutOfBounds) =
                                { "R", "0020", "Argument position out of bounds" };
ERR(PortNotFound) =             { "R", "0030", "Port not found" };
ERR(UsingOutputPortAsInput) =   { "R", "0040", "Using output port as input" };
ERR(UnresolvedReference) =      { "R", "0050", "Unresolved reference" };
ERR(UndefinedMember) =          { "R", "0060", "Undefined member" };
ERR(AccessingInternalMember) =  { "R", "0061", "Invalid reference to an internal member" };
ERR(CannotFindDefaultValue) =   { "R", "0070", "Cannot find default value" };
ERR(CircularReference) =        { "R", "0080", "Circular reference detected" };

// v - validation errors
ERR(InputSpecifiedMultipleTimes) = 
                                { "V", "0010", "Input specified multiple times" };
ERR(SymbolUsedMultipleTimes) =  { "V", "0030", "Symbol used multiple times" };
ERR(InputNotConnected) =        { "V", "0040", "Input not connected" };
ERR(OutputWithNoDefinition) =   { "V", "0050", "Output with no definition" };
ERR(BuiltinOutputWithDefinition) =
                                { "V", "0051", "Builtin output given a definition" };
ERR(BuiltinOutputMissingType) = { "V", "0052", "Builtin output missing a type" };
ERR(InputSpecifiedMultipleTimesPositional) =
                                { "V", "0011", "Input specified multiple times by position" };
ERR(DuplicateNodeDefinition) =  { "V", "0060", "Multiple definitions with the same name" };
ERR(UndefinedBuiltinType) =     { "V", "0070", "Undefined builtin type" };
ERR(UndefinedBuiltinInput) =
                                { "V", "0080", "Undefined builtin input" };
ERR(UndefinedBuiltinOutput) =
                                { "V", "0081", "Undefined builtin output" };
ERR(ModifyAttributeMismatch) =  { "V", "0090", "Input modify flag doesn't match builtin port" };
ERR(ToggleAttributeMismatch) =  { "V", "0100", "Input toggle flag doesn't match builtin port" };
ERR(AliasAttributeMismatch) =   { "V", "0110", "Alias flag doesn't match builtin port" };

// t - type errors
ERR(IncompatibleType) =         { "T", "0010", "Argument with incompatible type specified; valid conversion not found" };
ERR(IncompatibleDefaultType) =  { "T", "0011", "Default with incompatible type specified; valid conversion not found" };
ERR(IncompatibleOutputDefinitionType) =    
                                { "T", "0012", "Incompatible output definition type; valid conversion not found" };
