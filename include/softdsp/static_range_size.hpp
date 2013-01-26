#ifndef SOFTDSP_STATIC_RANGE_SIZE_HPP
#define SOFTDSP_STATIC_RANGE_SIZE_HPP

#include <array>
#include <boost/array.hpp>
#include <boost/mpl/size_t.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>

namespace softdsp {
  struct not_a_static_range { typedef not_a_static_range type; };
  template< typename Sequence, bool is_sequence = boost::fusion::traits::is_sequence< Sequence >::value >
  struct get_sequence_size {};
  template< typename Sequence >
  struct get_sequence_size< Sequence, true > : public boost::fusion::result_of::size< Sequence >::type {};
  template< typename Sequence >
  struct get_sequence_size< Sequence, false > : public not_a_static_range {};
  template< typename Sequence >
  get_sequence_size< Sequence > get_static_range_size(
    Sequence,
    typename boost::enable_if< boost::fusion::traits::is_sequence< Sequence > >::type* = 0
  );
  not_a_static_range get_static_range_size(
     ...
  );
  template< typename T >
  struct static_range_size : public decltype( get_static_range_size( std::declval< T >() ) ) {};
  template< typename T, size_t size_ >
  struct static_range_size< std::array< T, size_ > > : public boost::mpl::size_t< size_ > {};
  template< typename T, size_t size_ >
  struct static_range_size< T[ size_ ] > : public boost::mpl::size_t< size_ > {};
  template< typename T >
  struct is_static_range : public boost::mpl::not_< boost::is_same< typename static_range_size< T >::type, not_a_static_range > > {};
}

#endif

