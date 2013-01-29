#ifndef SOFTDSP_CONTEXT_HPP
#define SOFTDSP_CONTEXT_HPP

#include <softdsp/primitive.hpp>
#include <softdsp/constant_generator.hpp>
#include <softdsp/tag.hpp>
#include <softdsp/type_generator.hpp>
#include <softdsp/nth_type.hpp>
#include <softdsp/data_layout/load_value_at.hpp>
#include <softdsp/data_layout/store_value_at.hpp>
#include <softdsp/data_layout/common.hpp>
#include <softdsp/data_layout/proxy.hpp>
#include <softdsp/data_layout/get_element_type.hpp>
#include <softdsp/data_layout/array.hpp>
#include <softdsp/data_layout/tuple.hpp>
#include <softdsp/placeholders.hpp>
#include <softdsp/llvm_toolbox.hpp>

#include <llvm/ADT/ArrayRef.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/BasicBlock.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/DataLayout.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/TargetRegistry.h>
#include <array>
#include <memory>
#include <vector>
#include <string>
#include <boost/range/algorithm.hpp>
#include <boost/range/reference.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/boost_array.hpp>
#include <boost/fusion/include/copy.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/include/make_vector.hpp>
#include <boost/fusion/adapted/adt/adapt_adt.hpp>
#include <boost/fusion/mpl.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/preprocessor/enum.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/swap.hpp>
#include <boost/proto/proto.hpp>
#include <boost/container/flat_map.hpp>

#include <hermit/range_traits.hpp>
#include <softdsp/static_range_size.hpp>

#include <cxxabi.h>

namespace softdsp {
  namespace proto = boost::proto;
  using proto::argsns_::list2;
  using proto::exprns_::expr;

  template< typename function_type >
  class softdsp_context {
  public:
    typedef softdsp_context< function_type > context_type;
    typedef boost::function_types::result_type< function_type > result_type;
    typedef boost::function_types::parameter_types< function_type > parameter_types;
    softdsp_context(
      const std::shared_ptr< llvm_toolbox< function_type > > &tools_
    ) : tools( tools_ ) {}
    template< typename Expr, typename Enable = void > struct eval{};
    template< typename Expr > struct eval<
      Expr,
      typename boost::enable_if<
        proto::matches< Expr, proto::terminal< proto::_ > >
      >::type
    > {
      typedef decltype( std::declval< context_type& >().as_value( boost::proto::value( std::declval< Expr& >() ) ) ) result_type;
      result_type operator()( Expr &expr, context_type &context ) {
        return context.as_value( boost::proto::value( expr ) );
      }
    };

#define ONE_OR_MORE_UNARY_OPERANDS_ARE_LLVM_VALUE \
  boost::mpl::or_< \
    boost::is_convertible< ValueType, llvm::Value* > \
  >

#define ONE_OR_MORE_BINARY_OPERANDS_ARE_LLVM_VALUE \
  boost::mpl::or_< \
    boost::is_convertible< LeftType, llvm::Value* >, \
    boost::is_convertible< RightType, llvm::Value* > \
  >

#define SOFTDSP_ENABLE_BINARY_OPERATOR( name ) \
    template< typename Expr > struct eval< \
      Expr, \
      typename boost::enable_if< \
        proto::matches< Expr, proto:: name < proto::_, proto::_ > > \
      >::type \
    > { \
      typedef decltype( \
        std::declval< context_type& >(). name ( \
          proto::eval( boost::proto::left( std::declval< Expr& >() ), std::declval< context_type& >() ), \
          proto::eval( boost::proto::right( std::declval< Expr& >() ), std::declval< context_type& >() ) \
        ) \
      ) result_type; \
      result_type operator()( Expr &expr, context_type &context ) { \
        return context. name ( \
            proto::eval( boost::proto::left( expr ), context ), \
            proto::eval( boost::proto::right( expr ), context ) \
          ); \
      } \
    };

#define SOFTDSP_ENABLE_UNARY_OPERATOR( name ) \
    template< typename Expr > struct eval< \
      Expr, \
      typename boost::enable_if< \
        proto::matches< Expr, proto:: name < proto::_ > > \
      >::type \
    > { \
      typedef decltype( \
        std::declval< context_type& >(). name ( \
          proto::eval( boost::proto::left( std::declval< Expr& >() ), std::declval< context_type& >() ) \
        ) \
      ) result_type; \
      result_type operator()( Expr &expr, context_type &context ) { \
        return context. name ( \
            proto::eval( boost::proto::left( expr ), context ) \
          ); \
      } \
    };

SOFTDSP_ENABLE_UNARY_OPERATOR( dereference )

    template< typename ValueType >
    llvm::Value *dereference(
      ValueType value_,
      typename boost::enable_if<
        ONE_OR_MORE_UNARY_OPERANDS_ARE_LLVM_VALUE
      >::type* = 0
    ) {
      const auto value = as_llvm_value( value_ );
      if( value->getType()->isVectorTy() ) {
        return tools->ir_builder.CreateExtractElement( value, as_llvm_value( 0u ) );
      }
      else if( value->getType()->isArrayTy() ) {
        std::vector< unsigned int > args = { 0u };
        llvm::ArrayRef< unsigned int > args_ref( args );
        return tools->ir_builder.CreateExtractValue( value, args_ref );
      }
      else if( value->getType()->isStructTy() ) {
        std::vector< unsigned int > args = { 0u };
        llvm::ArrayRef< unsigned int > args_ref( args );
        return tools->ir_builder.CreateExtractValue( value, args_ref );
      }
      else if( value->getType()->isPointerTy() ) {
        return tools->ir_builder.CreateLoad( value );
      }
      else
        throw -1;
    }
    template< typename ValueType >
    auto dereference(
      ValueType value_,
      typename boost::disable_if<
        ONE_OR_MORE_UNARY_OPERANDS_ARE_LLVM_VALUE
      >::type* = 0
    ) -> decltype( *value_ ) {
      return *value_;
    }

SOFTDSP_ENABLE_BINARY_OPERATOR( plus )

    template< typename LeftType, typename RightType >
    llvm::Value *plus(
      LeftType left_, RightType right_,
      typename boost::enable_if<
        ONE_OR_MORE_BINARY_OPERANDS_ARE_LLVM_VALUE
      >::type* = 0
    ) {
      const auto left = as_llvm_value( left_ );
      const auto right = as_llvm_value( right_ );
      // need implicit cast
      if( left->getType()->getScalarType()->isIntegerTy() ) {
        return tools->ir_builder.CreateAdd( left, right );
      }
      else if( left->getType()->getScalarType()->isFloatingPointTy() ) {
        return tools->ir_builder.CreateFAdd( left, right );
      }
      else
        throw -1;
    }
    template< typename LeftType, typename RightType >
    auto plus(
      LeftType left_, RightType right_,
      typename boost::disable_if<
        ONE_OR_MORE_BINARY_OPERANDS_ARE_LLVM_VALUE
      >::type* = 0
    ) -> decltype( left_ + right_ ) {
      return left_ + right_;
    }

SOFTDSP_ENABLE_BINARY_OPERATOR( minus )

    template< typename LeftType, typename RightType >
    llvm::Value *minus(
      LeftType left_, RightType right_,
      typename boost::enable_if<
        ONE_OR_MORE_BINARY_OPERANDS_ARE_LLVM_VALUE
      >::type* = 0
    ) {
      const auto left = as_llvm_value( left_ );
      const auto right = as_llvm_value( right_ );
      // need implicit cast
      if( left->getType()->getScalarType()->isIntegerTy() ) {
        return tools->ir_builder.CreateSub( left, right );
      }
      else if( left->getType()->getScalarType()->isFloatingPointTy() ) {
        return tools->ir_builder.CreateFSub( left, right );
      }
      else
        throw -1;
    }
    template< typename LeftType, typename RightType >
    auto minus(
      LeftType left_, RightType right_,
      typename boost::disable_if<
        ONE_OR_MORE_BINARY_OPERANDS_ARE_LLVM_VALUE
      >::type* = 0
    ) -> decltype( left_ - right_ ) {
      return left_ - right_;
    }

SOFTDSP_ENABLE_BINARY_OPERATOR( subscript )

    template< typename LeftType, typename RightType >
    llvm::Value *subscript(
      LeftType left_, RightType right_,
      typename boost::enable_if<
        boost::mpl::or_<
          boost::fusion::traits::is_sequence< RightType >,
          boost::is_convertible< LeftType, llvm::Value* >
        >
      >::type* = 0
    ) {
      const auto left = as_llvm_value( left_ );
      // need implicit cast
      if( left->getType()->isVectorTy() ) {
        const auto right = as_llvm_value( right_ );
        return tools->ir_builder.CreateExtractElement( left, right );
      }
      else if( left->getType()->isArrayTy() ) {
        std::vector< unsigned int > args = { static_cast< unsigned int >( right_ ) };
        llvm::ArrayRef< unsigned int > args_ref( args );
        return tools->ir_builder.CreateExtractValue( left, args_ref );
      }
      else if( left->getType()->isStructTy() ) {
        std::vector< unsigned int > args = { static_cast< unsigned int >( right_ ) };
        llvm::ArrayRef< unsigned int > args_ref( args );
        return tools->ir_builder.CreateExtractValue( left, args_ref );
      }
      else if( left->getType()->isPointerTy() ) {
        
        throw -1.0;
      }
      else
        throw -1;
    }
    template< typename LeftType, typename RightType >
    auto subscript(
      LeftType left_, RightType right_,
      typename boost::disable_if<
        boost::mpl::or_<
          boost::fusion::traits::is_sequence< RightType >,
          boost::is_convertible< LeftType, llvm::Value* >
        >
      >::type* = 0
    ) -> decltype( left_[ right_ ] ) {
      return left_[ right_ ];
    }

  private:
    template< typename value_type >
    llvm::Value *as_llvm_value(
      const value_type &value,
      typename boost::disable_if<
        boost::is_convertible<
          llvm::Value*,
          value_type
        >
      >::type* = 0
    ) {
      llvm::Value *llvm_value = tools->constant_generator_( value );
      tools->extra_value_info.insert(
        std::make_pair(
          llvm_value,
          std::shared_ptr< value_info >( new value_info( boost::is_signed< value_type >::value ) )
        )
      );
      return llvm_value;
    }
    template< typename value_type >
    llvm::Value *as_llvm_value(
      const value_type &value,
      typename boost::enable_if<
        boost::is_convertible<
          llvm::Value*,
          value_type
        >
      >::type* = 0
    ) {
      return value;
    }
    template< typename value_type >
    value_type as_value(
      const value_type &value,
      typename boost::disable_if<
        boost::is_convertible<
          placeholder_,
          value_type
        >
      >::type* = 0
    ) {
      return value;
    }
    template< typename Index >
    llvm::Value *as_value(
      const placeholder< Index > &
    ) {
      return &*std::next( tools->llvm_function->getArgumentList().begin(), Index::value );
    }
    const std::shared_ptr< llvm_toolbox< function_type > > &tools;
  };
}

#endif
