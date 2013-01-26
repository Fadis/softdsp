#ifndef SOFTDSP_TYPE_VALUE_TRANSFORM_HPP
#define SOFTDSP_TYPE_VALUE_TRANSFORM_HPP

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/sequence/io/out.hpp>

namespace softdsp {
  struct end_of_sequence {};

  template< typename ... Types >
  class type_sequence {
  private:
    template< typename Head, typename ... Tail >
    struct head_type {
      typedef Head type; 
    };
    template< typename Head, typename ... Tail >
    struct tail_type {
      typedef type_sequence< Tail... > type;
    };
    template< typename Head >
    struct tail_type< Head > {
      typedef end_of_sequence type;
    };
  public:
    struct head {
      typedef typename head_type< Types... >::type type;
    };
    struct tail {
      typedef typename tail_type< Types... >::type type;
    };
  };


  template< typename Sequence, >
  struct fusion2type_sequence {
    
  };

  template< typename T >
  struct tag {};

  template< typename T, typename Func >
  struct result_of_transform {
    typedef decltype(
      boost::fusion::push_front(
        std::declval<
          typename result_of_transform<
            typename T::tail::type, Func
          >::type
        >(),
        std::declval< Func >()( std::declval< tag< typename T::head::type > >() )
      )
    ) type;
  };

  template< typename Func >
  struct result_of_transform< end_of_sequence, Func > {
    typedef boost::fusion::vector0<> type;
  };

  template< typename T, typename Func >
  typename result_of_transform< T, Func >::type transform(
    const Func &func,
    typename boost::disable_if<
      boost::is_same< T, end_of_sequence >
    >::type* = 0
  ) {
    return boost::fusion::push_front(
      transform< typename T::tail::type >( func ),
      func( tag< typename T::head::type >() )
    );
  }

  template< typename T, typename Func >
  typename result_of_transform< T, Func >::type transform(
    const Func &func,
    typename boost::enable_if<
      boost::is_same< T, end_of_sequence >
    >::type* = 0
  ) {
    return typename result_of_transform< T, Func >::type();
  }
}

#endif

