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

#include <hermit/range_traits.hpp>
#include <softdsp/static_range_size.hpp>

#include <cxxabi.h>

namespace softdsp {
  namespace proto = boost::proto;
  using proto::argsns_::list2;
  using proto::exprns_::expr;
  struct placeholder_ {};
  template < typename Index >
  struct placeholder : public placeholder_ {};
#define HERMIT_STREAM_PLACEHOLDER( z, index, unused ) \
  boost::proto::terminal< placeholder< boost::mpl::int_< index > > >::type const BOOST_PP_CAT( _, BOOST_PP_INC( index ) ) = {{}};
  BOOST_PP_REPEAT( FUSION_MAX_VECTOR_SIZE, HERMIT_STREAM_PLACEHOLDER, )

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
      llvm_function( llvm::Function::Create( llvm_function_type, llvm::Function::ExternalLinkage ) ) {
      ir_builder.SetInsertPoint(
        llvm::BasicBlock::Create( *llvm_context, name.c_str(), llvm_function )
      );
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
  };

  struct value {
    value( llvm::Value *value_, bool is_signed_ ) : llvm_value( value_ ), is_signed( is_signed_ ) {}
    llvm::Value * const llvm_value;
    const bool is_signed;
  };

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
      typedef llvm::Value *result_type;
      result_type operator()( Expr &expr, context_type &context ) {
        return context.as_value( boost::proto::value( expr ) );
      }
    };
    template< typename Expr > struct eval<
      Expr,
      typename boost::enable_if<
        proto::matches< Expr, proto::plus< proto::_, proto::_ > >
      >::type
    > {
      typedef llvm::Value *result_type;
      result_type operator()( Expr &expr, context_type &context ) {
        const auto left = proto::eval( boost::proto::left( expr ), context );
        const auto right = proto::eval( boost::proto::right( expr ), context );
        // need implicit cast
        if( left->getType()->getScalarType()->isIntegerTy() ) {
          return context.tools->ir_builder.CreateAdd( left, right );
        }
        else if( left->getType()->getScalarType()->isFloatingPointTy() ) {
          return context.tools->ir_builder.CreateFAdd( left, right );
        }
        else
          throw -1;
      }
    };
    template< typename Expr > struct eval<
      Expr,
      typename boost::enable_if<
        proto::matches< Expr, proto::minus< proto::_, proto::_ > >
      >::type
    > {
      typedef llvm::Value *result_type;
      result_type operator()( Expr &expr, context_type &context ) {
        const auto left = proto::eval( boost::proto::left( expr ), context );
        const auto right = proto::eval( boost::proto::right( expr ), context );
        // need implicit cast
        if( left->getType()->getScalarType()->isIntegerTy() ) {
          return context.tools->ir_builder.CreateSub( left, right );
        }
        else if( left->getType()->getScalarType()->isFloatingPointTy() ) {
          return context.tools->ir_builder.CreateFSub( left, right );
        }
        else
          throw -1;
      }
    };
  private:
    template< typename value_type >
    llvm::Value *as_value(
      const value_type &value,
      typename boost::disable_if<
        boost::is_convertible<
          placeholder_,
          value_type
        >
      >::type* = 0
    ) {
      return tools->constant_generator_( value );
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

int main() {
  using namespace softdsp;
  std::shared_ptr< llvm::LLVMContext > context( new llvm::LLVMContext() );
  llvm::Module *module = new llvm::Module( "foo_bar", *context );
  llvm::IRBuilder<> builder( *context );

  std::vector< llvm::Type* > args_raw( 2 );
  args_raw[ 0 ] = static_cast< llvm::Type* >( llvm::VectorType::get( builder.getFloatTy(), 4 ) );
  args_raw[ 1 ] = static_cast< llvm::Type* >( llvm::VectorType::get( builder.getFloatTy(), 4 ) );
  llvm::ArrayRef< llvm::Type* > args( args_raw );
  
  constant_generator gen( context );
  
  std::vector< float > const_vec = { 0.0f, 0.1f, 0.2f, 0.3f };
  llvm::Constant *const_vec_as_llvm = gen( const_vec );
  std::vector< int > const_int_vec = { 0, 1, 2, 3 };
  llvm::Constant *const_int_vec_as_llvm = gen( const_int_vec );
  auto const_int_vec_fp = builder.CreateSIToFP(
    const_int_vec_as_llvm,
    const_vec_as_llvm->getType()
  );

  const auto const_struct = boost::fusion::make_vector( 5, 'a', boost::fusion::make_vector( 1.0f, 3 ) );
  llvm::Constant *const_struct_as_llvm = gen( const_struct );
  type_generator ct( context );
  llvm::Type *a_type = ct( tag< boost::fusion::vector< int, std::array< float, 5 >, double > >() );
  llvm::DataLayout dl( module );
  std::cout << dl.getTypeStoreSize( a_type ) << std::endl;
  llvm::FunctionType *func1t =
    llvm::FunctionType::get( a_type, false );
  llvm::Function *func1 = 
    llvm::Function::Create( func1t, llvm::Function::ExternalLinkage, "foo", module );
  builder.SetInsertPoint(
    llvm::BasicBlock::Create( *context, "foo", func1 )
  );
  //builder.CreateRet( const_struct_as_llvm );

  llvm::FunctionType *func2t =
    llvm::FunctionType::get( llvm::VectorType::get( builder.getFloatTy(), 4 ), args, false );
  llvm::Function *func2 = 
    llvm::Function::Create( func2t, llvm::Function::ExternalLinkage );
  module->getFunctionList().push_back( func2 );
  builder.SetInsertPoint(
    llvm::BasicBlock::Create( *context, "entrypoint", func2 )
  );


  auto &arg_values = func2->getArgumentList();
  /*auto result = llvm::BinaryOperator::Create(
    llvm::Instruction::FAdd,
    static_cast< llvm::Value* >( &arg_values.front() ),
    static_cast< llvm::Value* >( &arg_values.back() )
  );*/
  auto result = builder.CreateFAdd(
    static_cast< llvm::Value* >( &arg_values.front() ),
    static_cast< llvm::Value* >( &arg_values.back() ) );
  auto result2 = builder.CreateFAdd(
    static_cast< llvm::Value* >( result ),
    static_cast< llvm::Value* >( const_vec_as_llvm ) );
  auto result3 = builder.CreateFAdd(
    static_cast< llvm::Value* >( result2 ),
    static_cast< llvm::Value* >( const_int_vec_fp ) );
  builder.CreateRet( result3 );
  module->dump();

  llvm::InitializeNativeTarget();
  std::string error;
  const llvm::Target * const target = llvm::TargetRegistry::lookupTarget( "x86_64-pc-linux", error );
  if( !error.empty() ) {
    std::cout << error << std::endl;
  }
  llvm::TargetOptions target_opts;
  target_opts.UseInitArray = 0;
  target_opts.UseSoftFloat = 0;
  target_opts.FloatABIType = llvm::FloatABI::Hard;
  const std::shared_ptr< llvm::TargetMachine > target_machine( target->createTargetMachine( "x86_64-pc-linux", "core2", "", target_opts ) );
  if( !target_machine ) {
    std::cout << "unable to create target machine." << std::endl;
  }
  const llvm::DataLayout *target_data = target_machine->getDataLayout();
  module->setTargetTriple( "x86_64-pc-linux" );
  module->setDataLayout( target_data->getStringRepresentation() );
  std::cout << module->getDataLayout() << std::endl;
  
  data_layout::tuple< int, data_layout::array< data_layout::tuple< int, double >, 20 >, float > dl0( context, module->getDataLayout() );
  int status;
  boost::fusion::at_c< 0 >( boost::fusion::at_c< 1 >( dl0 ).at( 0 ) ) = 1;
  std::cout << boost::fusion::at_c< 0 >( boost::fusion::at_c< 1 >( dl0 ).at( 0 ) ) << std::endl;
  boost::fusion::at_c< 2 >( dl0 ) = 0.5f;
  std::cout << boost::fusion::at_c< 2 >( dl0 ) << std::endl;

  std::shared_ptr< softdsp::llvm_toolbox< int ( int ) > > tools( new softdsp::llvm_toolbox< int ( int ) >( context, "foo" ) );
  softdsp_context< int ( int ) > sd_context( tools );
  module->getFunctionList().push_back( tools->llvm_function );
  tools->ir_builder.CreateRet( boost::proto::eval( proto::lit( 5 ) + 3 - 1 + softdsp::_1, sd_context ) );
  module->dump();
}

