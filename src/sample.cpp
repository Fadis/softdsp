#include <mcheck.h>
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
#include <softdsp/context.hpp>
#include <softdsp/static_cast.hpp>

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
#include <llvm/Support/raw_ostream.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/ExecutionEngine/JITEventListener.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/Support/IRReader.h>
#include <llvm/Support/SourceMgr.h>


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
#include <boost/function_types/function_pointer.hpp>
#include <boost/function_types/components.hpp>

#include <hermit/range_traits.hpp>
#include <softdsp/static_range_size.hpp>

#include <cxxabi.h>

namespace softdsp {

  template< typename function_type >
  class function {
  public:
    function(
      const std::shared_ptr< llvm::LLVMContext > &context_,
      const std::shared_ptr< llvm::EngineBuilder > builder_,
      const std::shared_ptr< llvm::ExecutionEngine > engine_,
      llvm::Function *entry_point_
    ) : context( context_ ), builder( builder_ ), engine( engine_ ), entry_point( entry_point_ ) {
    }
    int operator()( const data_layout::array< int, 2 > &value, const softdsp::data_layout::tuple< int, double, int, int > &value2 ) {
      //return reinterpret_cast< typename boost::function_types::function_pointer< boost::function_types::components< function_type > >::type >( engine->getPointerToFunction( entry_point ) )( value );
      return reinterpret_cast< int(*)( const void*, const void* ) >( engine->getPointerToFunction( entry_point ) )( value.get(), value2.get() );
    }
  private:
    const std::shared_ptr< llvm::LLVMContext > context;
    const std::shared_ptr< llvm::EngineBuilder > builder;
    const std::shared_ptr< llvm::ExecutionEngine > engine;
    llvm::Function *entry_point;
  };

  class executable {
  public:
    executable(
      const std::shared_ptr< llvm::LLVMContext > &context_,
      const std::shared_ptr< llvm::Module > &llvm_module_
    ) : context( context_ ), llvm_module( llvm::CloneModule( llvm_module_.get() ) ) {
      llvm::EngineBuilder *engine_builder = new llvm::EngineBuilder( llvm_module );
      std::shared_ptr< std::string > error_message( new std::string );
      engine_builder->setErrorStr( error_message.get() );
      engine_builder->setEngineKind( llvm::EngineKind::JIT );
      engine_builder->setOptLevel( llvm::CodeGenOpt::Aggressive );
      engine.reset( engine_builder->create() );
      if (!engine) {
        delete engine_builder;
        if (!error_message->empty())
          llvm::errs() << "dcompile::module" << ": error creating EE: " << *error_message << "\n";
        else
          llvm::errs() << "dcompile::module" << ": unknown error creating EE!\n";
        throw -1;
      }
      builder.reset( engine_builder, boost::bind( &executable::deleteBuilder, ::_1, engine, error_message ) );
      builder->setRelocationModel(llvm::Reloc::Default);
      builder->setCodeModel( llvm::CodeModel::JITDefault );
      builder->setUseMCJIT(true);
      engine->RegisterJITEventListener(llvm::JITEventListener::createOProfileJITEventListener());
      engine->DisableLazyCompilation(true);
      engine->runStaticConstructorsDestructors(false);
    }
    template< typename function_type >
    function< function_type > get_function( const std::string &name ) const {
      llvm::Function *entry_point = llvm_module->getFunction( name.c_str() );
      if( !entry_point )
        throw -1;
      return function< function_type >( context, builder, engine, entry_point );
    }
  private:
    static void deleteBuilder(
        llvm::EngineBuilder *builder,
        std::shared_ptr< llvm::ExecutionEngine > engine,
        std::shared_ptr< std::string >
      ) {
      engine->runStaticConstructorsDestructors(true);
      delete builder;
    }
    const std::shared_ptr< llvm::LLVMContext > context;
    const std::shared_ptr< llvm::Module > cloned_module;
    std::shared_ptr< llvm::EngineBuilder > builder;
    std::shared_ptr< llvm::ExecutionEngine > engine;
    llvm::Module *llvm_module;
  };

  class module {
  public:
    module(
      const std::shared_ptr< llvm::LLVMContext > &context_,
      const std::string &name_,
      const std::string &data_layout
    ) : llvm_context( context_ ),
        llvm_module( new llvm::Module( name_.c_str(), *llvm_context ) ) {
      llvm_module->setDataLayout( data_layout );
    }
    template < typename function_type, typename Expr >
    void add_function( const std::string name_, Expr &expr ) {
      std::shared_ptr< softdsp::llvm_toolbox< function_type > > tools( new softdsp::llvm_toolbox< function_type >( llvm_context, name_.c_str() ) );
      softdsp_context< function_type > context( tools );
      llvm_module->getFunctionList().push_back( tools->llvm_function );
      const auto result = boost::proto::eval( expr, context );
      int status;
      std::cout << abi::__cxa_demangle( typeid( result ).name(), 0, 0, &status ) << std::endl;
      result.value->getType()->dump();
      tools->ir_builder.CreateRet(
        tools->as_llvm_value(
          tools->load( result )
        ).value
      );
    }
    void dump() const {
      llvm_module->dump();
    }
    executable compile() const {
      return executable( llvm_context, llvm_module );
    }
  private:
    const std::shared_ptr< llvm::LLVMContext > llvm_context;
    const std::shared_ptr< llvm::Module > llvm_module;
  };

  class value_generator {
  public:
    value_generator(
      const std::shared_ptr< llvm::LLVMContext > &context_,
      const std::string &layout_
    ) : context( context_ ), layout( layout_ ) {}
    template< typename T >
    T operator()( tag< T > ) {
      const std::shared_ptr< data_layout::common > common_resources_(
        new data_layout::common( context, layout, tag< T >() )
      );
      return T(
        common_resources_,
        boost::iterator_range< uint8_t* >(
          common_resources_->data.data(),
          common_resources_->data.data() + common_resources_->data.size() 
        )
      );
    }
  private:
    const std::shared_ptr< llvm::LLVMContext > context;
    const std::string layout;
  };
}
int main() {
  using namespace softdsp;
  std::shared_ptr< llvm::LLVMContext > context( new llvm::LLVMContext() );
  llvm::Module *module = new llvm::Module( "foo_bar", *context );
  llvm::IRBuilder<> builder( *context );
/*
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
  //auto result = llvm::BinaryOperator::Create(
  //  llvm::Instruction::FAdd,
  //  static_cast< llvm::Value* >( &arg_values.front() ),
  //  static_cast< llvm::Value* >( &arg_values.back() )
  //);
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
 
  value_generator vg( context, module->getDataLayout() );
  auto dl0 = vg( tag< data_layout::tuple< int, data_layout::array< data_layout::tuple< int, double >, 20 >, float > >() );
//  data_layout::tuple< int, data_layout::array< data_layout::tuple< int, double >, 20 >, float > dl0( context, module->getDataLayout() );
  int status;
  boost::fusion::at_c< 0 >( boost::fusion::at_c< 1 >( dl0 ).at( 0 ) ) = 1;
  std::cout << boost::fusion::at_c< 0 >( boost::fusion::at_c< 1 >( dl0 ).at( 0 ) ) << std::endl;
  boost::fusion::at_c< 2 >( dl0 ) = 0.5f;
  std::cout << boost::fusion::at_c< 2 >( dl0 ) << std::endl;

  std::shared_ptr< softdsp::llvm_toolbox< int ( softdsp::data_layout::array< int, 5 > ) > > tools( new softdsp::llvm_toolbox< int ( softdsp::data_layout::array< int, 5 > ) >( context, "foo" ) );
  softdsp_context< int ( softdsp::data_layout::array< int, 5 > ) > sd_context( tools );
  module->getFunctionList().push_back( tools->llvm_function );
  tools->ir_builder.CreateRet( boost::proto::eval( proto::lit( 5 ) + 3 - 1 + softdsp::_1[ 1 ], sd_context ).value );
  module->dump();

  const auto roo = ct( tag< softdsp::data_layout::array< int, 5 > >() );
  roo->dump();
  const auto doo = ct( tag< std::array< int, 5 > >() );
  doo->dump();

  {
    softdsp::module sd_module( context, "the_module", target_data->getStringRepresentation() );
    sd_module.add_function< int ( softdsp::data_layout::array< int, 5 > ) >(
      "woo",
      proto::lit( 5 ) + 3 - 1 + softdsp::_1[ 1 ]
    );
    sd_module.dump();
    const auto executable = sd_module.compile();
  }*/
  {
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
  const auto foo = proto::lit( 10 ) + 0;
  boost::proto::display_expr( softdsp::static_cast_< int >( proto::lit( 10 ) + 0 ) );
  const llvm::DataLayout *target_data = target_machine->getDataLayout();
  module->setTargetTriple( "x86_64-pc-linux" );
  module->setDataLayout( target_data->getStringRepresentation() );
    softdsp::module sd_module( context, "the_module", target_data->getStringRepresentation() );
    sd_module.add_function< int ( softdsp::data_layout::array< int, 2 >, softdsp::data_layout::tuple< int, double, int, int > ) >(
      "woo",
      softdsp::static_cast_< int32_t >( softdsp::static_cast_< uint64_t >( (softdsp::_1)[ 0 ] ) + softdsp::static_cast_< uint64_t >( softdsp::at_c< 2 >( softdsp::_2 ) ) )
    );
    sd_module.dump();
    value_generator vg( context, module->getDataLayout() );
    auto arg1 = vg( tag< softdsp::data_layout::array< int, 2 > >() );
    arg1[ 0 ] = 1;
    arg1[ 1 ] = 2;
    auto arg2 = vg( tag< softdsp::data_layout::tuple< int, double, int, int > >() );
    boost::fusion::at_c< 0 >( arg2 ) = 4;
    boost::fusion::at_c< 1 >( arg2 ) = 5.0;
    boost::fusion::at_c< 2 >( arg2 ) = 8;
    boost::fusion::at_c< 3 >( arg2 ) = 20;
    std::cout << boost::fusion::at_c< 0 >( arg2 ) << std::endl;
    std::cout << boost::fusion::at_c< 1 >( arg2 ) << std::endl;
    std::cout << boost::fusion::at_c< 2 >( arg2 ) << std::endl;
    std::cout << boost::fusion::at_c< 3 >( arg2 ) << std::endl;
    std::shared_ptr< softdsp::llvm_toolbox< int ( softdsp::data_layout::array< int, 2 >, softdsp::data_layout::tuple< int, double, int, int > ) > > tools( new softdsp::llvm_toolbox< int ( softdsp::data_layout::array< int, 2 >, softdsp::data_layout::tuple< int, double, int, int > ) >( context, "foo" ) );
    softdsp_context< int ( softdsp::data_layout::array< int, 2 >, softdsp::data_layout::tuple< int, double, int, int > ) > sd_context( tools );
    const auto executable = sd_module.compile();
    auto function = executable.get_function< int ( softdsp::data_layout::array< int, 2 >, softdsp::data_layout::tuple< int, double, int, int > ) >( "woo" );
    std::cout << function( arg1,arg2 ) << std::endl;
  }
}

