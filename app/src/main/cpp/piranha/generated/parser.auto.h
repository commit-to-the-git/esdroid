// a bison parser made by gnu bison 3.8.2

// skeleton interface for bison lalr1 parsers in c++

// Copyright (C) 2002-2015, 2018-2021 Free Software Foundation, Inc.

// this program is free software you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the free software foundation either version 3 of the license or
// at your option any later version

// this program is distributed in the hope that it will be useful
// but without any warranty without even the implied warranty of
// merchantability or fitness for a particular purpose see the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// as a special exception you may create a larger work that contains
// part or all of the bison parser skeleton and distribute that work
// under terms of your choice so long as that work isnt itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton alternatively if you modify or redistribute
// the parser skeleton itself you may at your option remove this
// special exception which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// license without this special exception

// this special exception was added by the free software foundation in
// version 2.2 of bison


/**
 * * \file parser.auto.h
 * * define the piranha::parser class
*/

// c++ lalr1 parser skeleton written by akim demaille

// do not rely on features that are not documented in the manual
// especially those whose name start with yy_ or yy_ they are
// private implementation details that can be changed or removed

#ifndef YY_YY_HOME_Z_WORK_ESDROID_APP_SRC_MAIN_CPP_PIRANHA_GENERATED_PARSER_AUTO_H_INCLUDED
# define YY_YY_HOME_Z_WORK_ESDROID_APP_SRC_MAIN_CPP_PIRANHA_GENERATED_PARSER_AUTO_H_INCLUDED
// %code requires blocks
#line 8 "flex-bison/specification.y"

    namespace piranha {
        class IrCompilationUnit;
        class Scanner;
    }

    #include "../include/ir_compilation_unit.h"
    #include "../include/ir_node.h"
    #include "../include/ir_attribute_list.h"
    #include "../include/ir_attribute.h"
    #include "../include/ir_value.h"
    #include "../include/ir_value_constant.h"
    #include "../include/ir_binary_operator.h"
    #include "../include/ir_import_statement.h"
    #include "../include/ir_token_info.h"
    #include "../include/ir_node_definition.h"
    #include "../include/ir_attribute_definition.h"
    #include "../include/ir_attribute_definition_list.h"
    #include "../include/compilation_error.h"
    #include "../include/ir_structure_list.h"
    #include "../include/ir_visibility.h"
    #include "../include/ir_unary_operator.h"
    #include "../include/memory_tracker.h"

    #include <string>

    #ifndef YY_NULLPTR
    #if defined __cplusplus && 201103L <= __cplusplus
    #define YY_NULLPTR nullptr
    #else
    #define YY_NULLPTR 0
    #endif
    #endif

    # define YYLLOC_DEFAULT(Cur, Rhs, N)                    \
    do                                                      \
        if (N) {                                            \
            (Cur).combine(&YYRHSLOC(Rhs, 1));               \
            (Cur).combine(&YYRHSLOC(Rhs, N));               \
        }                                                   \
        else {                                              \
            (Cur).combine(&YYRHSLOC(Rhs, 0));               \
        }                                                   \
    while (0)

    /* remove annoying compiler warnings */
    #ifdef _MSC_VER
    /* warning c4065 switch statement contains default but no case labels */
    #pragma warning (disable: 4065)
    #endif

#line 101 "parser.auto.h"

# include <cassert>
# include <cstdlib> //  std::abort
# include <iostream>
# include <stdexcept>
# include <string>
# include <vector>

#if defined __cplusplus
# define YY_CPLUSPLUS __cplusplus
#else
# define YY_CPLUSPLUS 199711L
#endif

// support move semantics when possible
#if 201103L <= YY_CPLUSPLUS
# define YY_MOVE           std::move
# define YY_MOVE_OR_COPY   move
# define YY_MOVE_REF(Type) Type&&
# define YY_RVREF(Type)    Type&&
# define YY_COPY(Type)     Type
#else
# define YY_MOVE
# define YY_MOVE_OR_COPY   copy
# define YY_MOVE_REF(Type) Type&
# define YY_RVREF(Type)    const Type&
# define YY_COPY(Type)     const Type&
#endif

// support noexcept when possible
#if 201103L <= YY_CPLUSPLUS
# define YY_NOEXCEPT noexcept
# define YY_NOTHROW
#else
# define YY_NOEXCEPT
# define YY_NOTHROW throw ()
#endif

// support constexpr when possible
#if 201703 <= YY_CPLUSPLUS
# define YY_CONSTEXPR constexpr
#else
# define YY_CONSTEXPR
#endif

#include <typeinfo>
#ifndef YY_ASSERT
# include <cassert>
# define YY_ASSERT assert
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* suppress unused-variable warnings by using e */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* suppress an incorrect diagnostic about yylval being uninitialized */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* nothing */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* debug traces */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

#line 5 "flex-bison/specification.y"
namespace piranha {
#line 242 "parser.auto.h"




  // / a bison parser
  class Parser
  {
  public:
#ifdef YYSTYPE
# ifdef __GNUC__
#  pragma GCC message "bison: do not #define YYSTYPE in C++, use %define api.value.type"
# endif
    typedef YYSTYPE value_type;
#else
  // / a buffer to store and retrieve objects
  // /
  // / sort of a variant but does not keep track of the nature
  // / of the stored data since that knowledge is available
  // / via the current parser state
  class value_type
  {
  public:
    // / type of *this
    typedef value_type self_type;

    // / empty construction
    value_type () YY_NOEXCEPT
      : yyraw_ ()
      , yytypeid_ (YY_NULLPTR)
    {}

    // / construct and fill
    template <typename T>
    value_type (YY_RVREF (T) t)
      : yytypeid_ (&typeid (T))
    {
      YY_ASSERT (sizeof (T) <= size);
      new (yyas_<T> ()) T (YY_MOVE (t));
    }

#if 201103L <= YY_CPLUSPLUS
    // / non copyable
    value_type (const self_type&) = delete;
    // / non copyable
    self_type& operator= (const self_type&) = delete;
#endif

    // / destruction allowed only if empty
    ~value_type () YY_NOEXCEPT
    {
      YY_ASSERT (!yytypeid_);
    }

# if 201103L <= YY_CPLUSPLUS
    // / instantiate a \a t in here from \a t
    template <typename T, typename... U>
    T&
    emplace (U&&... u)
    {
      YY_ASSERT (!yytypeid_);
      YY_ASSERT (sizeof (T) <= size);
      yytypeid_ = & typeid (T);
      return *new (yyas_<T> ()) T (std::forward <U>(u)...);
    }
# else
    // / instantiate an empty \a t in here
    template <typename T>
    T&
    emplace ()
    {
      YY_ASSERT (!yytypeid_);
      YY_ASSERT (sizeof (T) <= size);
      yytypeid_ = & typeid (T);
      return *new (yyas_<T> ()) T ();
    }

    // / instantiate a \a t in here from \a t
    template <typename T>
    T&
    emplace (const T& t)
    {
      YY_ASSERT (!yytypeid_);
      YY_ASSERT (sizeof (T) <= size);
      yytypeid_ = & typeid (T);
      return *new (yyas_<T> ()) T (t);
    }
# endif

    // / instantiate an empty \a t in here
    // / obsolete use emplace
    template <typename T>
    T&
    build ()
    {
      return emplace<T> ();
    }

    // / instantiate a \a t in here from \a t
    // / obsolete use emplace
    template <typename T>
    T&
    build (const T& t)
    {
      return emplace<T> (t);
    }

    // / accessor to a built \a t
    template <typename T>
    T&
    as () YY_NOEXCEPT
    {
      YY_ASSERT (yytypeid_);
      YY_ASSERT (*yytypeid_ == typeid (T));
      YY_ASSERT (sizeof (T) <= size);
      return *yyas_<T> ();
    }

    // / const accessor to a built \a t for %printer
    template <typename T>
    const T&
    as () const YY_NOEXCEPT
    {
      YY_ASSERT (yytypeid_);
      YY_ASSERT (*yytypeid_ == typeid (T));
      YY_ASSERT (sizeof (T) <= size);
      return *yyas_<T> ();
    }

    // / swap the content with \a that of same type
    // /
    // / both variants must be built beforehand because swapping the actual
    // / data requires reading it with as and this is not possible on
    // / unconstructed variants it would require some dynamic testing which
    // / should not be the variants responsibility
    // / swapping between built and possibly non-built is done with
    // / self_type::move
    template <typename T>
    void
    swap (self_type& that) YY_NOEXCEPT
    {
      YY_ASSERT (yytypeid_);
      YY_ASSERT (*yytypeid_ == *that.yytypeid_);
      std::swap (as<T> (), that.as<T> ());
    }

    // / move the content of \a that to this
    // /
    // / destroys \a that
    template <typename T>
    void
    move (self_type& that)
    {
# if 201103L <= YY_CPLUSPLUS
      emplace<T> (std::move (that.as<T> ()));
# else
      emplace<T> ();
      swap<T> (that);
# endif
      that.destroy<T> ();
    }

# if 201103L <= YY_CPLUSPLUS
    // / move the content of \a that to this
    template <typename T>
    void
    move (self_type&& that)
    {
      emplace<T> (std::move (that.as<T> ()));
      that.destroy<T> ();
    }
#endif

    // / copy the content of \a that to this
    template <typename T>
    void
    copy (const self_type& that)
    {
      emplace<T> (that.as<T> ());
    }

    // / destroy the stored \a t
    template <typename T>
    void
    destroy ()
    {
      as<T> ().~T ();
      yytypeid_ = YY_NULLPTR;
    }

  private:
#if YY_CPLUSPLUS < 201103L
    // / non copyable
    value_type (const self_type&);
    // / non copyable
    self_type& operator= (const self_type&);
#endif

    // / accessor to raw memory as \a t
    template <typename T>
    T*
    yyas_ () YY_NOEXCEPT
    {
      void *yyp = yyraw_;
      return static_cast<T*> (yyp);
     }

    // / const accessor to raw memory as \a t
    template <typename T>
    const T*
    yyas_ () const YY_NOEXCEPT
    {
      const void *yyp = yyraw_;
      return static_cast<const T*> (yyp);
     }

    // / an auxiliary type to compute the largest semantic type
    union union_type
    {
      // attribute
      char dummy1[sizeof (piranha::IrAttribute *)];

      //  port_declaration
      //  port_status
      //  port_value
      //  port_connection
      //  documented_port_definition
      char dummy2[sizeof (piranha::IrAttributeDefinition *)];

      //  port_definitions
      char dummy3[sizeof (piranha::IrAttributeDefinitionList *)];

      //  connection_block
      //  attribute_list
      char dummy4[sizeof (piranha::IrAttributeList *)];

      //  import_statement
      //  import_statement_visibility
      //  import_statement_short_name
      char dummy5[sizeof (piranha::IrImportStatement *)];

      // node
      //  node_member
      //  inline_node_member
      //  inline_node
      char dummy6[sizeof (piranha::IrNode *)];

      //  node_name
      //  node_inline
      //  node_shadow
      //  node_definition
      //  specific_node_definition
      //  immediate_node_definition
      //  node_decorator
      char dummy7[sizeof (piranha::IrNodeDefinition *)];

      //  node_list
      char dummy8[sizeof (piranha::IrNodeList *)];

      //  type_name_namespace
      char dummy9[sizeof (piranha::IrTokenInfoSet<std::string, 2>)];

      // bool
      char dummy10[sizeof (piranha::IrTokenInfo_bool)];

      // float
      char dummy11[sizeof (piranha::IrTokenInfo_float)];

      // int
      char dummy12[sizeof (piranha::IrTokenInfo_int)];

      // char
      // import
      // as
      // node
      // inline
      // alias
      // input
      // output
      // modify
      // toggle
      // label
      // string
      // decorator
      // public
      // private
      //  BUILTIN_POINTER
      //  NAMESPACE_POINTER
      // unrecognized
      // operator
      // module
      // auto
      //
      // +
      //
      // /
      //
      //
      //
      // {
      // }
      //
      //
      //
      //
      //
      //
      // ^
      //  type_name
      //  standard_operator
      // string
      char dummy13[sizeof (piranha::IrTokenInfo_string)];

      //  label_value
      // value
      // constant
      //  atomic_value
      //  primary_exp
      //  data_access
      //  unary_exp
      //  mul_exp
      //  add_exp
      char dummy14[sizeof (piranha::IrValue *)];
    };

    // / the size of the largest semantic type
    enum { size = sizeof (union_type) };

    // / a buffer to store semantic values
    union
    {
      // / strongest alignment constraints
      long double yyalign_me_;
      // / a buffer large enough to store any of the semantic values
      char yyraw_[size];
    };

    // / whether the content is built if defined the name of the stored type
    const std::type_info *yytypeid_;
  };

#endif
    // / backward compatibility bison 3.8
    typedef value_type semantic_type;

    // / symbol locations
    typedef piranha::IrTokenInfo location_type;

    // / syntax errors thrown from user actions
    struct syntax_error : std::runtime_error
    {
      syntax_error (const location_type& l, const std::string& m)
        : std::runtime_error (m)
        , location (l)
      {}

      syntax_error (const syntax_error& s)
        : std::runtime_error (s.what ())
        , location (s.location)
      {}

      ~syntax_error () YY_NOEXCEPT YY_NOTHROW;

      location_type location;
    };

    // / token kinds
    struct token
    {
      enum token_kind_type
      {
        YYEMPTY = -2,
    END = 0,                       // end
    YYerror = 256,                 // error
    YYUNDEF = 257,                 // invalid token
    CHAR = 258,                    // char
    IMPORT = 259,                  // import
    AS = 260,                      // as
    NODE = 261,                    // node
    INLINE = 262,                  // inline
    ALIAS = 263,                   // alias
    INPUT = 264,                   // input
    OUTPUT = 265,                  // output
    MODIFY = 266,                  // modify
    TOGGLE = 267,                  // toggle
    LABEL = 268,                   // label
    INT = 269,                     // int
    FLOAT = 270,                   // float
    BOOL = 271,                    // bool
    STRING = 272,                  // string
    DECORATOR = 273,               // decorator
    PUBLIC = 274,                  // public
    PRIVATE = 275,                 // private
    BUILTIN_POINTER = 276,         //  BUILTIN_POINTER
    NAMESPACE_POINTER = 277,       //  NAMESPACE_POINTER
    UNRECOGNIZED = 278,            // unrecognized
    OPERATOR = 279,                // operator
    MODULE = 280,                  // module
    AUTO = 281                     // auto
      };
      // / backward compatibility alias bison 3.6
      typedef token_kind_type yytokentype;
    };

    // / token kind as returned by yylex
    typedef token::token_kind_type token_kind_type;

    // / backward compatibility alias bison 3.6
    typedef token_kind_type token_type;

    // / symbol kinds
    struct symbol_kind
    {
      enum symbol_kind_type
      {
        YYNTOKENS = 44, // /< number of tokens
        S_YYEMPTY = -2,
        S_YYEOF = 0,                             // end
        S_YYerror = 1,                           // error
        S_YYUNDEF = 2,                           // invalid token
        S_CHAR = 3,                              // char
        S_IMPORT = 4,                            // import
        S_AS = 5,                                // as
        S_NODE = 6,                              // node
        S_INLINE = 7,                            // inline
        S_ALIAS = 8,                             // alias
        S_INPUT = 9,                             // input
        S_OUTPUT = 10,                           // output
        S_MODIFY = 11,                           // modify
        S_TOGGLE = 12,                           // toggle
        S_LABEL = 13,                            // label
        S_INT = 14,                              // int
        S_FLOAT = 15,                            // float
        S_BOOL = 16,                             // bool
        S_STRING = 17,                           // string
        S_DECORATOR = 18,                        // decorator
        S_PUBLIC = 19,                           // public
        S_PRIVATE = 20,                          // private
        S_BUILTIN_POINTER = 21,                  //  BUILTIN_POINTER
        S_NAMESPACE_POINTER = 22,                //  NAMESPACE_POINTER
        S_UNRECOGNIZED = 23,                     // unrecognized
        S_OPERATOR = 24,                         // operator
        S_MODULE = 25,                           // module
        S_AUTO = 26,                             // auto
        S_27_ = 27,                              //
        S_28_ = 28,                              // +
        S_29_ = 29,                              //
        S_30_ = 30,                              // /
        S_31_ = 31,                              //
        S_32_ = 32,                              //
        S_33_ = 33,                              //
        S_34_ = 34,                              // {
        S_35_ = 35,                              // }
        S_36_ = 36,                              //
        S_37_ = 37,                              //
        S_38_ = 38,                              //
        S_39_ = 39,                              //
        S_40_ = 40,                              //
        S_41_ = 41,                              //
        S_42_ = 42,                              // ^
        S_43_ = 43,                              //
        S_YYACCEPT = 44,                         // $accept
        S_sdl = 45,                              // sdl
        S_decorator = 46,                        // decorator
        S_decorator_list = 47,                   //  decorator_list
        S_statement = 48,                        // statement
        S_statement_list = 49,                   //  statement_list
        S_import_statement = 50,                 //  import_statement
        S_import_statement_visibility = 51,      //  import_statement_visibility
        S_import_statement_short_name = 52,      //  import_statement_short_name
        S_type_name = 53,                        //  type_name
        S_type_name_namespace = 54,              //  type_name_namespace
        S_node = 55,                             // node
        S_node_member = 56,                      //  node_member
        S_node_list = 57,                        //  node_list
        S_standard_operator = 58,                //  standard_operator
        S_node_name = 59,                        //  node_name
        S_node_inline = 60,                      //  node_inline
        S_node_shadow = 61,                      //  node_shadow
        S_node_definition = 62,                  //  node_definition
        S_specific_node_definition = 63,         //  specific_node_definition
        S_immediate_node_definition = 64,        //  immediate_node_definition
        S_node_decorator = 65,                   //  node_decorator
        S_port_definitions = 66,                 //  port_definitions
        S_port_declaration = 67,                 //  port_declaration
        S_port_status = 68,                      //  port_status
        S_port_value = 69,                       //  port_value
        S_port_connection = 70,                  //  port_connection
        S_documented_port_definition = 71,       //  documented_port_definition
        S_inline_node_member = 72,               //  inline_node_member
        S_inline_node = 73,                      //  inline_node
        S_connection_block = 74,                 //  connection_block
        S_attribute_list = 75,                   //  attribute_list
        S_attribute = 76,                        // attribute
        S_label_value = 77,                      //  label_value
        S_value = 78,                            // value
        S_string = 79,                           // string
        S_constant = 80,                         // constant
        S_atomic_value = 81,                     //  atomic_value
        S_primary_exp = 82,                      //  primary_exp
        S_data_access = 83,                      //  data_access
        S_unary_exp = 84,                        //  unary_exp
        S_mul_exp = 85,                          //  mul_exp
        S_add_exp = 86                           //  add_exp
      };
    };

    // / internal symbol kind
    typedef symbol_kind::symbol_kind_type symbol_kind_type;

    // / the number of tokens
    static const symbol_kind_type YYNTOKENS = symbol_kind::YYNTOKENS;

    // / a complete symbol
    // /
    // / expects its base type to provide access to the symbol kind
    // / via kind
    // /
    // / provide access to semantic value and location
    template <typename Base>
    struct basic_symbol : Base
    {
      // / alias to base
      typedef Base super_type;

      // / default constructor
      basic_symbol () YY_NOEXCEPT
        : value ()
        , location ()
      {}

#if 201103L <= YY_CPLUSPLUS
      // / move constructor
      basic_symbol (basic_symbol&& that)
        : Base (std::move (that))
        , value ()
        , location (std::move (that.location))
      {
        switch (this->kind ())
    {
      case symbol_kind::S_attribute: // attribute
        value.move< piranha::IrAttribute * > (std::move (that.value));
        break;

      case symbol_kind::S_port_declaration: //  port_declaration
      case symbol_kind::S_port_status: //  port_status
      case symbol_kind::S_port_value: //  port_value
      case symbol_kind::S_port_connection: //  port_connection
      case symbol_kind::S_documented_port_definition: //  documented_port_definition
        value.move< piranha::IrAttributeDefinition * > (std::move (that.value));
        break;

      case symbol_kind::S_port_definitions: //  port_definitions
        value.move< piranha::IrAttributeDefinitionList * > (std::move (that.value));
        break;

      case symbol_kind::S_connection_block: //  connection_block
      case symbol_kind::S_attribute_list: //  attribute_list
        value.move< piranha::IrAttributeList * > (std::move (that.value));
        break;

      case symbol_kind::S_import_statement: //  import_statement
      case symbol_kind::S_import_statement_visibility: //  import_statement_visibility
      case symbol_kind::S_import_statement_short_name: //  import_statement_short_name
        value.move< piranha::IrImportStatement * > (std::move (that.value));
        break;

      case symbol_kind::S_node: // node
      case symbol_kind::S_node_member: //  node_member
      case symbol_kind::S_inline_node_member: //  inline_node_member
      case symbol_kind::S_inline_node: //  inline_node
        value.move< piranha::IrNode * > (std::move (that.value));
        break;

      case symbol_kind::S_node_name: //  node_name
      case symbol_kind::S_node_inline: //  node_inline
      case symbol_kind::S_node_shadow: //  node_shadow
      case symbol_kind::S_node_definition: //  node_definition
      case symbol_kind::S_specific_node_definition: //  specific_node_definition
      case symbol_kind::S_immediate_node_definition: //  immediate_node_definition
      case symbol_kind::S_node_decorator: //  node_decorator
        value.move< piranha::IrNodeDefinition * > (std::move (that.value));
        break;

      case symbol_kind::S_node_list: //  node_list
        value.move< piranha::IrNodeList * > (std::move (that.value));
        break;

      case symbol_kind::S_type_name_namespace: //  type_name_namespace
        value.move< piranha::IrTokenInfoSet<std::string, 2> > (std::move (that.value));
        break;

      case symbol_kind::S_BOOL: // bool
        value.move< piranha::IrTokenInfo_bool > (std::move (that.value));
        break;

      case symbol_kind::S_FLOAT: // float
        value.move< piranha::IrTokenInfo_float > (std::move (that.value));
        break;

      case symbol_kind::S_INT: // int
        value.move< piranha::IrTokenInfo_int > (std::move (that.value));
        break;

      case symbol_kind::S_CHAR: // char
      case symbol_kind::S_IMPORT: // import
      case symbol_kind::S_AS: // as
      case symbol_kind::S_NODE: // node
      case symbol_kind::S_INLINE: // inline
      case symbol_kind::S_ALIAS: // alias
      case symbol_kind::S_INPUT: // input
      case symbol_kind::S_OUTPUT: // output
      case symbol_kind::S_MODIFY: // modify
      case symbol_kind::S_TOGGLE: // toggle
      case symbol_kind::S_LABEL: // label
      case symbol_kind::S_STRING: // string
      case symbol_kind::S_DECORATOR: // decorator
      case symbol_kind::S_PUBLIC: // public
      case symbol_kind::S_PRIVATE: // private
      case symbol_kind::S_BUILTIN_POINTER: //  BUILTIN_POINTER
      case symbol_kind::S_NAMESPACE_POINTER: //  NAMESPACE_POINTER
      case symbol_kind::S_UNRECOGNIZED: // unrecognized
      case symbol_kind::S_OPERATOR: // operator
      case symbol_kind::S_MODULE: // module
      case symbol_kind::S_AUTO: // auto
      case symbol_kind::S_27_: //
      case symbol_kind::S_28_: // +
      case symbol_kind::S_29_: //
      case symbol_kind::S_30_: // /
      case symbol_kind::S_31_: //
      case symbol_kind::S_32_: //
      case symbol_kind::S_33_: //
      case symbol_kind::S_34_: // {
      case symbol_kind::S_35_: // }
      case symbol_kind::S_36_: //
      case symbol_kind::S_37_: //
      case symbol_kind::S_38_: //
      case symbol_kind::S_39_: //
      case symbol_kind::S_40_: //
      case symbol_kind::S_41_: //
      case symbol_kind::S_42_: // ^
      case symbol_kind::S_type_name: //  type_name
      case symbol_kind::S_standard_operator: //  standard_operator
      case symbol_kind::S_string: // string
        value.move< piranha::IrTokenInfo_string > (std::move (that.value));
        break;

      case symbol_kind::S_label_value: //  label_value
      case symbol_kind::S_value: // value
      case symbol_kind::S_constant: // constant
      case symbol_kind::S_atomic_value: //  atomic_value
      case symbol_kind::S_primary_exp: //  primary_exp
      case symbol_kind::S_data_access: //  data_access
      case symbol_kind::S_unary_exp: //  unary_exp
      case symbol_kind::S_mul_exp: //  mul_exp
      case symbol_kind::S_add_exp: //  add_exp
        value.move< piranha::IrValue * > (std::move (that.value));
        break;

      default:
        break;
    }

      }
#endif

      // / copy constructor
      basic_symbol (const basic_symbol& that);

      // / constructors for typed symbols
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, location_type&& l)
        : Base (t)
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const location_type& l)
        : Base (t)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, piranha::IrAttribute *&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const piranha::IrAttribute *& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, piranha::IrAttributeDefinition *&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const piranha::IrAttributeDefinition *& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, piranha::IrAttributeDefinitionList *&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const piranha::IrAttributeDefinitionList *& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, piranha::IrAttributeList *&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const piranha::IrAttributeList *& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, piranha::IrImportStatement *&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const piranha::IrImportStatement *& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, piranha::IrNode *&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const piranha::IrNode *& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, piranha::IrNodeDefinition *&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const piranha::IrNodeDefinition *& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, piranha::IrNodeList *&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const piranha::IrNodeList *& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, piranha::IrTokenInfoSet<std::string, 2>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const piranha::IrTokenInfoSet<std::string, 2>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, piranha::IrTokenInfo_bool&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const piranha::IrTokenInfo_bool& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, piranha::IrTokenInfo_float&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const piranha::IrTokenInfo_float& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, piranha::IrTokenInfo_int&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const piranha::IrTokenInfo_int& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, piranha::IrTokenInfo_string&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const piranha::IrTokenInfo_string& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, piranha::IrValue *&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const piranha::IrValue *& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

      // / destroy the symbol
      ~basic_symbol ()
      {
        clear ();
      }



      // / destroy contents and record that is empty
      void clear () YY_NOEXCEPT
      {
        // user destructor
        symbol_kind_type yykind = this->kind ();
        basic_symbol<Base>& yysym = *this;
        (void) yysym;
        switch (yykind)
        {
       default:
          break;
        }

        // value type destructor
switch (yykind)
    {
      case symbol_kind::S_attribute: // attribute
        value.template destroy< piranha::IrAttribute * > ();
        break;

      case symbol_kind::S_port_declaration: //  port_declaration
      case symbol_kind::S_port_status: //  port_status
      case symbol_kind::S_port_value: //  port_value
      case symbol_kind::S_port_connection: //  port_connection
      case symbol_kind::S_documented_port_definition: //  documented_port_definition
        value.template destroy< piranha::IrAttributeDefinition * > ();
        break;

      case symbol_kind::S_port_definitions: //  port_definitions
        value.template destroy< piranha::IrAttributeDefinitionList * > ();
        break;

      case symbol_kind::S_connection_block: //  connection_block
      case symbol_kind::S_attribute_list: //  attribute_list
        value.template destroy< piranha::IrAttributeList * > ();
        break;

      case symbol_kind::S_import_statement: //  import_statement
      case symbol_kind::S_import_statement_visibility: //  import_statement_visibility
      case symbol_kind::S_import_statement_short_name: //  import_statement_short_name
        value.template destroy< piranha::IrImportStatement * > ();
        break;

      case symbol_kind::S_node: // node
      case symbol_kind::S_node_member: //  node_member
      case symbol_kind::S_inline_node_member: //  inline_node_member
      case symbol_kind::S_inline_node: //  inline_node
        value.template destroy< piranha::IrNode * > ();
        break;

      case symbol_kind::S_node_name: //  node_name
      case symbol_kind::S_node_inline: //  node_inline
      case symbol_kind::S_node_shadow: //  node_shadow
      case symbol_kind::S_node_definition: //  node_definition
      case symbol_kind::S_specific_node_definition: //  specific_node_definition
      case symbol_kind::S_immediate_node_definition: //  immediate_node_definition
      case symbol_kind::S_node_decorator: //  node_decorator
        value.template destroy< piranha::IrNodeDefinition * > ();
        break;

      case symbol_kind::S_node_list: //  node_list
        value.template destroy< piranha::IrNodeList * > ();
        break;

      case symbol_kind::S_type_name_namespace: //  type_name_namespace
        value.template destroy< piranha::IrTokenInfoSet<std::string, 2> > ();
        break;

      case symbol_kind::S_BOOL: // bool
        value.template destroy< piranha::IrTokenInfo_bool > ();
        break;

      case symbol_kind::S_FLOAT: // float
        value.template destroy< piranha::IrTokenInfo_float > ();
        break;

      case symbol_kind::S_INT: // int
        value.template destroy< piranha::IrTokenInfo_int > ();
        break;

      case symbol_kind::S_CHAR: // char
      case symbol_kind::S_IMPORT: // import
      case symbol_kind::S_AS: // as
      case symbol_kind::S_NODE: // node
      case symbol_kind::S_INLINE: // inline
      case symbol_kind::S_ALIAS: // alias
      case symbol_kind::S_INPUT: // input
      case symbol_kind::S_OUTPUT: // output
      case symbol_kind::S_MODIFY: // modify
      case symbol_kind::S_TOGGLE: // toggle
      case symbol_kind::S_LABEL: // label
      case symbol_kind::S_STRING: // string
      case symbol_kind::S_DECORATOR: // decorator
      case symbol_kind::S_PUBLIC: // public
      case symbol_kind::S_PRIVATE: // private
      case symbol_kind::S_BUILTIN_POINTER: //  BUILTIN_POINTER
      case symbol_kind::S_NAMESPACE_POINTER: //  NAMESPACE_POINTER
      case symbol_kind::S_UNRECOGNIZED: // unrecognized
      case symbol_kind::S_OPERATOR: // operator
      case symbol_kind::S_MODULE: // module
      case symbol_kind::S_AUTO: // auto
      case symbol_kind::S_27_: //
      case symbol_kind::S_28_: // +
      case symbol_kind::S_29_: //
      case symbol_kind::S_30_: // /
      case symbol_kind::S_31_: //
      case symbol_kind::S_32_: //
      case symbol_kind::S_33_: //
      case symbol_kind::S_34_: // {
      case symbol_kind::S_35_: // }
      case symbol_kind::S_36_: //
      case symbol_kind::S_37_: //
      case symbol_kind::S_38_: //
      case symbol_kind::S_39_: //
      case symbol_kind::S_40_: //
      case symbol_kind::S_41_: //
      case symbol_kind::S_42_: // ^
      case symbol_kind::S_type_name: //  type_name
      case symbol_kind::S_standard_operator: //  standard_operator
      case symbol_kind::S_string: // string
        value.template destroy< piranha::IrTokenInfo_string > ();
        break;

      case symbol_kind::S_label_value: //  label_value
      case symbol_kind::S_value: // value
      case symbol_kind::S_constant: // constant
      case symbol_kind::S_atomic_value: //  atomic_value
      case symbol_kind::S_primary_exp: //  primary_exp
      case symbol_kind::S_data_access: //  data_access
      case symbol_kind::S_unary_exp: //  unary_exp
      case symbol_kind::S_mul_exp: //  mul_exp
      case symbol_kind::S_add_exp: //  add_exp
        value.template destroy< piranha::IrValue * > ();
        break;

      default:
        break;
    }

        Base::clear ();
      }

#if YYDEBUG || 0
      // / the user-facing name of this symbol
      const char *name () const YY_NOEXCEPT
      {
        return Parser::symbol_name (this->kind ());
      }
#endif // #if yydebug || 0


      // / backward compatibility bison 3.6
      symbol_kind_type type_get () const YY_NOEXCEPT;

      // / whether empty
      bool empty () const YY_NOEXCEPT;

      // / destructive move \a s is emptied into this
      void move (basic_symbol& s);

      // / the semantic value
      value_type value;

      // / the location
      location_type location;

    private:
#if YY_CPLUSPLUS < 201103L
      // / assignment operator
      basic_symbol& operator= (const basic_symbol& that);
#endif
    };

    // / type access provider for token enum based symbols
    struct by_kind
    {
      // / the symbol kind as needed by the constructor
      typedef token_kind_type kind_type;

      // / default constructor
      by_kind () YY_NOEXCEPT;

#if 201103L <= YY_CPLUSPLUS
      // / move constructor
      by_kind (by_kind&& that) YY_NOEXCEPT;
#endif

      // / copy constructor
      by_kind (const by_kind& that) YY_NOEXCEPT;

      // / constructor from external token numbers
      by_kind (kind_type t) YY_NOEXCEPT;



      // / record that this symbol is empty
      void clear () YY_NOEXCEPT;

      // / steal the symbol kind from \a that
      void move (by_kind& that);

      // / the internal type number corresponding to \a type
      // / \a empty when empty
      symbol_kind_type kind () const YY_NOEXCEPT;

      // / backward compatibility bison 3.6
      symbol_kind_type type_get () const YY_NOEXCEPT;

      // / the symbol kind
      // / \a s_yyempty when empty
      symbol_kind_type kind_;
    };

    // / backward compatibility for a private implementation detail bison 3.6
    typedef by_kind by_type;

    // / external symbols returned by the scanner
    struct symbol_type : basic_symbol<by_kind>
    {
      // / superclass
      typedef basic_symbol<by_kind> super_type;

      // / empty symbol
      symbol_type () YY_NOEXCEPT {}

      // / constructor for valueless symbols and symbols from each type
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, location_type l)
        : super_type (token_kind_type (tok), std::move (l))
#else
      symbol_type (int tok, const location_type& l)
        : super_type (token_kind_type (tok), l)
#endif
      {
#if !defined _MSC_VER || defined __clang__
        YY_ASSERT (tok == token::END
                   || (token::YYerror <= tok && tok <= token::YYUNDEF)
                   || tok == 33);
#endif
      }
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, piranha::IrTokenInfo_bool v, location_type l)
        : super_type (token_kind_type (tok), std::move (v), std::move (l))
#else
      symbol_type (int tok, const piranha::IrTokenInfo_bool& v, const location_type& l)
        : super_type (token_kind_type (tok), v, l)
#endif
      {
#if !defined _MSC_VER || defined __clang__
        YY_ASSERT (tok == token::BOOL);
#endif
      }
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, piranha::IrTokenInfo_float v, location_type l)
        : super_type (token_kind_type (tok), std::move (v), std::move (l))
#else
      symbol_type (int tok, const piranha::IrTokenInfo_float& v, const location_type& l)
        : super_type (token_kind_type (tok), v, l)
#endif
      {
#if !defined _MSC_VER || defined __clang__
        YY_ASSERT (tok == token::FLOAT);
#endif
      }
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, piranha::IrTokenInfo_int v, location_type l)
        : super_type (token_kind_type (tok), std::move (v), std::move (l))
#else
      symbol_type (int tok, const piranha::IrTokenInfo_int& v, const location_type& l)
        : super_type (token_kind_type (tok), v, l)
#endif
      {
#if !defined _MSC_VER || defined __clang__
        YY_ASSERT (tok == token::INT);
#endif
      }
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, piranha::IrTokenInfo_string v, location_type l)
        : super_type (token_kind_type (tok), std::move (v), std::move (l))
#else
      symbol_type (int tok, const piranha::IrTokenInfo_string& v, const location_type& l)
        : super_type (token_kind_type (tok), v, l)
#endif
      {
#if !defined _MSC_VER || defined __clang__
        YY_ASSERT ((token::CHAR <= tok && tok <= token::LABEL)
                   || (token::STRING <= tok && tok <= token::AUTO)
                   || tok == 61
                   || tok == 43
                   || tok == 45
                   || tok == 47
                   || tok == 42
                   || (40 <= tok && tok <= 41)
                   || tok == 123
                   || tok == 125
                   || tok == 91
                   || tok == 93
                   || (58 <= tok && tok <= 59)
                   || tok == 44
                   || tok == 46
                   || tok == 94);
#endif
      }
    };

    // / build a parser object
    Parser (Scanner &scanner_yyarg, IrCompilationUnit &driver_yyarg);
    virtual ~Parser ();

#if 201103L <= YY_CPLUSPLUS
    // / non copyable
    Parser (const Parser&) = delete;
    // / non copyable
    Parser& operator= (const Parser&) = delete;
#endif

    // / parse an alias for parse
    // / \returns 0 iff parsing succeeded
    int operator() ();

    // / parse
    // / \returns 0 iff parsing succeeded
    virtual int parse ();

#if YYDEBUG
    // / the current debugging stream
    std::ostream& debug_stream () const YY_ATTRIBUTE_PURE;
    // / set the current debugging stream
    void set_debug_stream (std::ostream &);

    // / type for debugging levels
    typedef int debug_level_type;
    // / the current debugging level
    debug_level_type debug_level () const YY_ATTRIBUTE_PURE;
    // / set the current debugging level
    void set_debug_level (debug_level_type l);
#endif

    // / report a syntax error
    // / \param loc where the syntax error is found
    // / \param msg a description of the syntax error
    virtual void error (const location_type& loc, const std::string& msg);

    // / report a syntax error
    void error (const syntax_error& err);

#if YYDEBUG || 0
    // / the user-facing name of the symbol whose internal number is
    // / yysymbol no bounds checking
    static const char *symbol_name (symbol_kind_type yysymbol);
#endif // #if yydebug || 0


    // implementation of make_symbol for each token kind
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_END (location_type l)
      {
        return symbol_type (token::END, std::move (l));
      }
#else
      static
      symbol_type
      make_END (const location_type& l)
      {
        return symbol_type (token::END, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_YYerror (location_type l)
      {
        return symbol_type (token::YYerror, std::move (l));
      }
#else
      static
      symbol_type
      make_YYerror (const location_type& l)
      {
        return symbol_type (token::YYerror, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_YYUNDEF (location_type l)
      {
        return symbol_type (token::YYUNDEF, std::move (l));
      }
#else
      static
      symbol_type
      make_YYUNDEF (const location_type& l)
      {
        return symbol_type (token::YYUNDEF, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CHAR (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::CHAR, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_CHAR (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::CHAR, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_IMPORT (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::IMPORT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_IMPORT (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::IMPORT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_AS (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::AS, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_AS (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::AS, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NODE (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::NODE, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_NODE (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::NODE, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INLINE (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::INLINE, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_INLINE (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::INLINE, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ALIAS (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::ALIAS, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_ALIAS (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::ALIAS, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INPUT (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::INPUT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_INPUT (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::INPUT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_OUTPUT (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::OUTPUT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_OUTPUT (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::OUTPUT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_MODIFY (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::MODIFY, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_MODIFY (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::MODIFY, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_TOGGLE (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::TOGGLE, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_TOGGLE (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::TOGGLE, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LABEL (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::LABEL, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_LABEL (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::LABEL, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INT (piranha::IrTokenInfo_int v, location_type l)
      {
        return symbol_type (token::INT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_INT (const piranha::IrTokenInfo_int& v, const location_type& l)
      {
        return symbol_type (token::INT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_FLOAT (piranha::IrTokenInfo_float v, location_type l)
      {
        return symbol_type (token::FLOAT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_FLOAT (const piranha::IrTokenInfo_float& v, const location_type& l)
      {
        return symbol_type (token::FLOAT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_BOOL (piranha::IrTokenInfo_bool v, location_type l)
      {
        return symbol_type (token::BOOL, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_BOOL (const piranha::IrTokenInfo_bool& v, const location_type& l)
      {
        return symbol_type (token::BOOL, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_STRING (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::STRING, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_STRING (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::STRING, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DECORATOR (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::DECORATOR, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_DECORATOR (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::DECORATOR, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PUBLIC (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::PUBLIC, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_PUBLIC (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::PUBLIC, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PRIVATE (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::PRIVATE, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_PRIVATE (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::PRIVATE, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_BUILTIN_POINTER (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::BUILTIN_POINTER, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_BUILTIN_POINTER (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::BUILTIN_POINTER, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NAMESPACE_POINTER (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::NAMESPACE_POINTER, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_NAMESPACE_POINTER (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::NAMESPACE_POINTER, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_UNRECOGNIZED (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::UNRECOGNIZED, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_UNRECOGNIZED (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::UNRECOGNIZED, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_OPERATOR (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::OPERATOR, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_OPERATOR (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::OPERATOR, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_MODULE (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::MODULE, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_MODULE (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::MODULE, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_AUTO (piranha::IrTokenInfo_string v, location_type l)
      {
        return symbol_type (token::AUTO, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_AUTO (const piranha::IrTokenInfo_string& v, const location_type& l)
      {
        return symbol_type (token::AUTO, v, l);
      }
#endif


  private:
#if YY_CPLUSPLUS < 201103L
    // / non copyable
    Parser (const Parser&);
    // / non copyable
    Parser& operator= (const Parser&);
#endif


    // / stored state numbers used for stacks
    typedef unsigned char state_type;

    // / compute post-reduction state
    // / \param yystate the current state
    // / \param yysym the nonterminal to push on the stack
    static state_type yy_lr_goto_state_ (state_type yystate, int yysym);

    // / whether the given \c yypact_ value indicates a defaulted state
    // / \param yyvalue the value to check
    static bool yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT;

    // / whether the given \c yytable_ value indicates a syntax error
    // / \param yyvalue the value to check
    static bool yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT;

    static const signed char yypact_ninf_;
    static const signed char yytable_ninf_;

    // / convert a scanner token kind \a t to a symbol kind
    // / in theory \a t should be a token_kind_type but character literals
    // / are valid yet not members of the token_kind_type enum
    static symbol_kind_type yytranslate_ (int t) YY_NOEXCEPT;

#if YYDEBUG || 0
    // / for a symbol its name in clear
    static const char* const yytname_[];
#endif // #if yydebug || 0


    // tables
    // yypactstate-num -- index in yytable of the portion describing
    // state-num
    static const short yypact_[];

    // yydefactstate-num -- default reduction number in state state-num
    // performed when yytable does not specify something else to do zero
    // means the default is an error
    static const signed char yydefact_[];

    // yypgotonterm-num
    static const short yypgoto_[];

    // yydefgotonterm-num
    static const unsigned char yydefgoto_[];

    // yytableyypactstate-num -- what to do in state state-num if
    // positive shift that token if negative reduce the rule whose
    // number is the opposite if yytable_ninf syntax error
    static const short yytable_[];

    static const short yycheck_[];

    // yystosstate-num -- the symbol kind of the accessing symbol of
    // state state-num
    static const signed char yystos_[];

    // yyr1rule-num -- symbol kind of the left-hand side of rule rule-num
    static const signed char yyr1_[];

    // yyr2rule-num -- number of symbols on the right-hand side of rule rule-num
    static const signed char yyr2_[];


#if YYDEBUG
    // yyrlineyyn -- source line where rule number yyn was defined
    static const short yyrline_[];
    // / report on the debug stream that the rule \a r is going to be reduced
    virtual void yy_reduce_print_ (int r) const;
    // / print the state stack on the debug stream
    virtual void yy_stack_print_ () const;

    // / debugging level
    int yydebug_;
    // / debug stream
    std::ostream* yycdebug_;

    // / \brief display a symbol kind value and location
    // / \param yyo the output stream
    // / \param yysym the symbol
    template <typename Base>
    void yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const;
#endif

    // / \brief reclaim the memory associated to a symbol
    // / \param yymsg why this token is reclaimed
    // / if null print nothing
    // / \param yysym the symbol
    template <typename Base>
    void yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const;

  private:
    // / type access provider for state based symbols
    struct by_state
    {
      // / default constructor
      by_state () YY_NOEXCEPT;

      // / the symbol kind as needed by the constructor
      typedef state_type kind_type;

      // / constructor
      by_state (kind_type s) YY_NOEXCEPT;

      // / copy constructor
      by_state (const by_state& that) YY_NOEXCEPT;

      // / record that this symbol is empty
      void clear () YY_NOEXCEPT;

      // / steal the symbol kind from \a that
      void move (by_state& that);

      // / the symbol kind corresponding to \a state
      // / \a symbol_kind::s_yyempty when empty
      symbol_kind_type kind () const YY_NOEXCEPT;

      // / the state number used to denote an empty symbol
      // / we use the initial state as it does not have a value
      enum { empty_state = 0 };

      // / the state
      // / \a empty when empty
      state_type state;
    };

    // / internal symbol element of the stack
    struct stack_symbol_type : basic_symbol<by_state>
    {
      // / superclass
      typedef basic_symbol<by_state> super_type;
      // / construct an empty symbol
      stack_symbol_type ();
      // / move or copy construction
      stack_symbol_type (YY_RVREF (stack_symbol_type) that);
      // / steal the contents from \a sym to build this
      stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) sym);
#if YY_CPLUSPLUS < 201103L
      // / assignment needed by push_back by some old implementations
      // / moves the contents of that
      stack_symbol_type& operator= (stack_symbol_type& that);

      // / assignment needed by push_back by other implementations
      // / needed by some other old implementations
      stack_symbol_type& operator= (const stack_symbol_type& that);
#endif
    };

    // / a stack with random access from its top
    template <typename T, typename S = std::vector<T> >
    class stack
    {
    public:
      // hide our reversed order
      typedef typename S::iterator iterator;
      typedef typename S::const_iterator const_iterator;
      typedef typename S::size_type size_type;
      typedef typename std::ptrdiff_t index_type;

      stack (size_type n = 200) YY_NOEXCEPT
        : seq_ (n)
      {}

#if 201103L <= YY_CPLUSPLUS
      // / non copyable
      stack (const stack&) = delete;
      // / non copyable
      stack& operator= (const stack&) = delete;
#endif

      // / random access
      // /
      // / index 0 returns the topmost element
      const T&
      operator[] (index_type i) const
      {
        return seq_[size_type (size () - 1 - i)];
      }

      // / random access
      // /
      // / index 0 returns the topmost element
      T&
      operator[] (index_type i)
      {
        return seq_[size_type (size () - 1 - i)];
      }

      // / steal the contents of \a t
      // /
      // / close to move-semantics
      void
      push (YY_MOVE_REF (T) t)
      {
        seq_.push_back (T ());
        operator[] (0).move (t);
      }

      // / pop elements from the stack
      void
      pop (std::ptrdiff_t n = 1) YY_NOEXCEPT
      {
        for (; 0 < n; --n)
          seq_.pop_back ();
      }

      // / pop all elements from the stack
      void
      clear () YY_NOEXCEPT
      {
        seq_.clear ();
      }

      // / number of elements on the stack
      index_type
      size () const YY_NOEXCEPT
      {
        return index_type (seq_.size ());
      }

      // / iterator on top of the stack going downwards
      const_iterator
      begin () const YY_NOEXCEPT
      {
        return seq_.begin ();
      }

      // / bottom of the stack
      const_iterator
      end () const YY_NOEXCEPT
      {
        return seq_.end ();
      }

      // / present a slice of the top of a stack
      class slice
      {
      public:
        slice (const stack& stack, index_type range) YY_NOEXCEPT
          : stack_ (stack)
          , range_ (range)
        {}

        const T&
        operator[] (index_type i) const
        {
          return stack_[range_ - i];
        }

      private:
        const stack& stack_;
        index_type range_;
      };

    private:
#if YY_CPLUSPLUS < 201103L
      // / non copyable
      stack (const stack&);
      // / non copyable
      stack& operator= (const stack&);
#endif
      // / the wrapped container
      S seq_;
    };


    // / stack type
    typedef stack<stack_symbol_type> stack_type;

    // / the stack
    stack_type yystack_;

    // / push a new state on the stack
    // / \param m a debug message to display
    // / if null no trace is output
    // / \param sym the symbol
    // / \warning the contents of \a s.value is stolen
    void yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym);

    // / push a new look ahead token on the state on the stack
    // / \param m a debug message to display
    // / if null no trace is output
    // / \param s the state
    // / \param sym the symbol for its value and location
    // / \warning the contents of \a sym.value is stolen
    void yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym);

    // / pop \a n symbols from the stack
    void yypop_ (int n = 1) YY_NOEXCEPT;

    // / constants
    enum
    {
      yylast_ = 391,     // /< last index in yytable_
      yynnts_ = 43,  // /< number of nonterminal symbols
      yyfinal_ = 80 // /< termination state number
    };


    // user arguments
    Scanner &scanner;
    IrCompilationUnit &driver;

  };


#line 5 "flex-bison/specification.y"
} // piranha
#line 2206 "parser.auto.h"




#endif //  !YY_YY_HOME_Z_WORK_ESDROID_APP_SRC_MAIN_CPP_PIRANHA_GENERATED_PARSER_AUTO_H_INCLUDED
