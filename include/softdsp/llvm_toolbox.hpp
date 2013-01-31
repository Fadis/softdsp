#ifndef SOFTDSP_LLVM_TOOLBOX_HPP
#define SOFTDSP_LLVM_TOOLBOX_HPP

#include <softdsp/constant_generator.hpp>
#include <softdsp/tag.hpp>
#include <softdsp/type_generator.hpp>
#include <softdsp/return_value.hpp>

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
#include <memory>
#include <vector>
#include <string>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/container/flat_map.hpp>

namespace softdsp {
  template< typename parameter_types >
  std::vector< llvm::Type* > generate_parameter_types(
      const type_generator &type_generator_,
      typename boost::enable_if<
        boost::mpl::not_< boost::mpl::empty< parameter_types > >
      >::type* = 0
    ) {
    auto llvm_types = generate_parameter_types< typename boost::mpl::pop_back< parameter_types >::type >( type_generator_ );
    llvm_types.push_back( type_generator_( tag< typename boost::mpl::back< parameter_types >::type >() ) );
    return llvm_types;
  }
  template< typename parameter_types >
  std::vector< llvm::Type* > generate_parameter_types(
      const type_generator &,
      typename boost::enable_if<
        boost::mpl::empty< parameter_types >
      >::type* = 0
    ) {
    return std::vector< llvm::Type* >();
  }

  struct value_info {
    value_info( bool is_signed_ ) : is_signed( is_signed_ ) {};
    const bool is_signed;
  };

  template< typename parameter_types >
  std::vector< std::shared_ptr< value_info > > generate_parameter_info(
      typename boost::enable_if<
        boost::mpl::empty< parameter_types >
      >::type* = 0
    ) {
    return std::vector< std::shared_ptr< value_info > >();
  }
  template< typename parameter_types >
  std::vector< std::shared_ptr< value_info > > generate_parameter_info(
      typename boost::enable_if<
        boost::mpl::not_< boost::mpl::empty< parameter_types > >
      >::type* = 0
    ) {
    auto info = generate_parameter_info< typename boost::mpl::pop_back< parameter_types >::type >();
    info.emplace_back(
      new value_info( boost::is_signed< typename boost::mpl::back< parameter_types >::type >::value )
    );
    return info;
  }

  template< typename function_type >
  struct llvm_toolbox {
    typedef typename boost::function_types::result_type< function_type >::type result_type;
    typedef typename boost::function_types::parameter_types< function_type >::type parameter_types;
    llvm_toolbox(
      const std::shared_ptr< llvm::LLVMContext > &context_,
      const std::string &name
    ) : llvm_context( context_ ),
      constant_generator_( llvm_context ),
      type_generator_( llvm_context ),
      ir_builder( *llvm_context ),
      llvm_return_type( type_generator_( tag< result_type >() ) ),
      llvm_parameter_types( generate_parameter_types< parameter_types >( type_generator_ ) ),
      llvm_parameter_types_ref( llvm_parameter_types ),
      llvm_function_type( llvm::FunctionType::get( llvm_return_type, llvm_parameter_types_ref,  false ) ),
      llvm_function( llvm::Function::Create( llvm_function_type, llvm::Function::ExternalLinkage, name.c_str() ) ) {
      const auto parameter_info = generate_parameter_info< parameter_types >();
      auto iter = llvm_function->getArgumentList().begin();
      auto index = 0u;
      for( ; iter != llvm_function->getArgumentList().end(); ++index, ++iter ) {
        extra_value_info.insert( std::make_pair( static_cast<llvm::Value*>( &*iter ), parameter_info[ index ] ) );
      }
      ir_builder.SetInsertPoint(
        llvm::BasicBlock::Create( *llvm_context, name.c_str(), llvm_function )
      );
    }
    template< typename value_type >
    return_value< value_type > as_llvm_value(
      const value_type &value,
      typename boost::disable_if<
        boost::is_convertible<
          value_type,
          return_value_
        >
      >::type* = 0
    ) {
      llvm::Value *llvm_value = constant_generator_( value );
      extra_value_info.insert(
        std::make_pair(
          llvm_value,
          std::shared_ptr< value_info >( new value_info( boost::is_signed< value_type >::value ) )
        )
      );
      return return_value< value_type >( llvm_value );
    }
    template< typename value_type >
    value_type as_llvm_value(
      const value_type &value,
      typename boost::enable_if<
        boost::is_convertible<
          value_type,
          return_value_
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
    return_value<
      typename boost::mpl::at<
        boost::function_types::parameter_types< function_type >,
        Index
      >::type
    > as_value(
      const placeholder< Index > &
    ) {
      llvm::Value *value = &*std::next( llvm_function->getArgumentList().begin(), Index::value );
      return return_value<
        typename boost::mpl::at<
          boost::function_types::parameter_types< function_type >,
          Index
        >::type
      >( value );
    }
    const std::shared_ptr< llvm::LLVMContext > llvm_context;
    const constant_generator constant_generator_;
    const type_generator type_generator_;
    llvm::IRBuilder<> ir_builder;
    llvm::Type *llvm_return_type;
    std::vector< llvm::Type* > llvm_parameter_types;
    llvm::ArrayRef< llvm::Type* > llvm_parameter_types_ref;
    llvm::FunctionType *llvm_function_type;
    llvm::Function *llvm_function;
    boost::container::flat_map< llvm::Value*, std::shared_ptr< value_info > > extra_value_info;
  };
}

#endif
