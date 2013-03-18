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
#include <softdsp/value_generator.hpp>
#include <softdsp/argument_generator.hpp>

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
  };

  template< typename return_type, typename ...parameters >
  class function< return_type( parameters... ) > {
  public:
    function(
      const std::shared_ptr< llvm::LLVMContext > &context_,
      const std::shared_ptr< llvm::EngineBuilder > builder_,
      const std::shared_ptr< llvm::ExecutionEngine > engine_,
      llvm::Function *entry_point_
    ) : context( context_ ), builder( builder_ ), engine( engine_ ), entry_point( entry_point_ ) {
    }
    return_type operator()( const data_layout::array< int, 2 > &value, const softdsp::data_layout::tuple< int, double, int, int > &value2, const data_layout::array< data_layout::array< int, 2 >, 2 > &value3 ) {
      return reinterpret_cast< return_type(*)( const void*, const void*, const void* ) >( engine->getPointerToFunction( entry_point ) )( value.get(), value2.get(), value3.get() );
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
    template< typename function_decl >
    function< typename function_decl::function_type > get_function() const {
      const auto function_name = std::string( "00sd" )+typeid( function_decl ).name();
      llvm::Function *entry_point = llvm_module->getFunction( function_name.c_str() );
      if( !entry_point )
        throw -1;
      return function< typename function_decl::function_type >( context, builder, engine, entry_point );
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

  template< typename function_decl >
  class function_ {
  public:
    function_(
      const std::shared_ptr< llvm::LLVMContext > &context_,
      const std::shared_ptr< llvm::Module > &module_
    ) : llvm_context( context_ ), llvm_module( module_ ) {}
    template< typename Expr >
    void operator[]( Expr &expr ) const {
      std::shared_ptr< softdsp::llvm_toolbox< typename function_decl::function_type > > tools(
        new softdsp::llvm_toolbox< typename function_decl::function_type >( llvm_context, ( std::string( "00sd" )+typeid( function_decl ).name() ).c_str() )
      );
      softdsp_context< typename function_decl::function_type > context( tools );
      llvm_module->getFunctionList().push_back( tools->llvm_function );
      const auto result = boost::proto::eval( expr, context );
      result.value->getType()->dump();
      tools->ir_builder.CreateRet(
        tools->as_llvm_value(
          tools->load( result )
        ).value
      );
    }
  private:
    const std::shared_ptr< llvm::LLVMContext > llvm_context;
    const std::shared_ptr< llvm::Module > llvm_module;
  };

  class native_target {
  public:
    static const native_target &initialize() {
      static const native_target instance;
      return instance;
    }
  private:
    native_target() {
      llvm::InitializeNativeTarget();
    }
  };

  class module {
  public:
    module(
      const std::shared_ptr< llvm::LLVMContext > &context_,
      const std::string &name_,
      const std::string &data_layout
    ) : llvm_context( context_ ), name( name_ ),
        llvm_module( new llvm::Module( name_.c_str(), *llvm_context ) ) {
      llvm_module->setDataLayout( data_layout );
    }
    module(
      const std::shared_ptr< llvm::LLVMContext > &context_,
      const std::string &name_,
      const std::string &triple,
      const std::string &cpu,
      const std::string &feature,
      const llvm::TargetOptions &opts
    ) : llvm_context( context_ ), name( name_ ) {
      softdsp::native_target::initialize();
      std::string error;
      const llvm::Target * const target = llvm::TargetRegistry::lookupTarget( triple.c_str(), error );
      if( !error.empty() ) {
        std::cout << error << std::endl;
        throw -1;
      }
      target_machine.reset(
        target->createTargetMachine( triple.c_str(), cpu.c_str(), feature.c_str(), opts )
      );
      if( !target_machine ) {
        std::cout << "unable to create target machine." << std::endl;
        throw -1;
      }
      const llvm::DataLayout *target_data = target_machine->getDataLayout();
      llvm_module.reset( new llvm::Module( name_, *context_ ) );
      llvm_module->setTargetTriple( triple.c_str() );
      llvm_module->setDataLayout( target_data->getStringRepresentation() );
    }
    template< typename function_decl >
    function_< function_decl > create_function() {
      return function_< function_decl >( llvm_context, llvm_module );
    }
    void dump() const {
      llvm_module->dump();
    }
    executable compile() const {
      return executable( llvm_context, llvm_module );
    }
    value_generator get_value_generator() {
      return value_generator( llvm_context, llvm_module->getDataLayout() );
    }
  private:
    const std::shared_ptr< llvm::LLVMContext > llvm_context;
    std::shared_ptr< llvm::Module > llvm_module;
    std::shared_ptr< llvm::TargetMachine > target_machine;
    llvm::DataLayout *target_data;
    const std::string name;
  };
}
struct woo {
  typedef int function_type( softdsp::data_layout::array< int, 2 >, softdsp::data_layout::tuple< int, double, int, int >, softdsp::data_layout::array< softdsp::data_layout::array< int, 2 >, 2 > );
};
int main() {
  using namespace softdsp;
  std::shared_ptr< llvm::LLVMContext > context( new llvm::LLVMContext() );
  llvm::TargetOptions target_opts;
  target_opts.UseInitArray = 0;
  target_opts.UseSoftFloat = 0;
  target_opts.FloatABIType = llvm::FloatABI::Hard;
  softdsp::module sd_module( context, "the_module", "x86_64-pc-linux", "core2", "", target_opts );
  sd_module.create_function< woo >()[
    softdsp::_1[ 0 ] <<= 20,
    softdsp::_1[ 1 ] = ( softdsp::static_cast_< int32_t >( softdsp::static_cast_< float >( softdsp::_1[ 0 ] ) * 3 + ++--softdsp::at_c< 2 >( softdsp::_2 ) / 3 ) + ( ~softdsp::_3[ 1 ][ 1 ] << 1 ) ) % 6
  ];
  sd_module.dump();
  value_generator vg = sd_module.get_value_generator();
  auto arg1 = vg( tag< softdsp::data_layout::array< int, 2 > >() );
  arg1[ 0 ] = 1;
  arg1[ 1 ] = 2;
  auto arg2 = vg( tag< softdsp::data_layout::tuple< int, double, int, int > >() );
  boost::fusion::at_c< 0 >( arg2 ) = 4;
  boost::fusion::at_c< 1 >( arg2 ) = 5.0;
  boost::fusion::at_c< 2 >( arg2 ) = 8;
  boost::fusion::at_c< 3 >( arg2 ) = 20;
  auto arg3 = vg( tag< converted_type< std::array< std::array< int, 2 >, 2 > >::type >() );
  arg3[ 0 ][ 0 ] = 1;
  arg3[ 0 ][ 1 ] = 2;
  arg3[ 1 ][ 0 ] = 3;
  arg3[ 1 ][ 1 ] = 4;
  const auto executable = sd_module.compile();
  auto function = executable.get_function< woo >();
  std::cout << function( arg1,arg2,arg3 ) << std::endl;
  int status;
  std::cout << abi::__cxa_demangle( typeid( converted_type< std::array< std::array< int, 2 >, 2 > >::type ).name(), 0, 0, &status ) << std::endl;
}

