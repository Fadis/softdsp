#ifndef SOFTDSP_DATA_LAYOUT_STORE_VALUE_AT_HPP
#define SOFTDSP_DATA_LAYOUT_STORE_VALUE_AT_HPP

#include <boost/range/algorithm.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/spirit/include/karma.hpp>

namespace softdsp {
  namespace data_layout {
  template< typename T > 
  void store_value_at(
    T value,
    uint8_t *at, bool is_big_endian,
    typename boost::enable_if<
      boost::mpl::or_<
        boost::is_same< typename boost::remove_cv< T >::type, int8_t >,
        boost::is_same< typename boost::remove_cv< T >::type, uint8_t >
      >
    >::type* = 0
  ) {
    namespace karma = boost::spirit::karma;
    if( karma::generate( at, karma::byte_, value ) ) return;
    else throw -1;
  }
  template< typename T > 
  void store_value_at(
    T value,
    uint8_t *at, bool is_big_endian,
    typename boost::enable_if<
      boost::mpl::or_<
        boost::is_same< typename boost::remove_cv< T >::type, int16_t >,
        boost::is_same< typename boost::remove_cv< T >::type, uint16_t >
      >
    >::type* = 0
  ) {
    namespace karma = boost::spirit::karma;
    if( is_big_endian ) {
      if( karma::generate( at, karma::big_word, value ) ) return;
      else throw -1;
    }
    else {
      if( karma::generate( at, karma::little_word, value ) ) return;
      else throw -1;
    }
  }
  template< typename T > 
  void store_value_at(
    T value,
    uint8_t *at, bool is_big_endian,
    typename boost::enable_if<
      boost::mpl::or_<
        boost::is_same< typename boost::remove_cv< T >::type, int32_t >,
        boost::is_same< typename boost::remove_cv< T >::type, uint32_t >
      >
    >::type* = 0
  ) {
    namespace karma = boost::spirit::karma;
    if( is_big_endian ) {
      if( karma::generate( at, karma::big_dword, value ) ) return;
      else throw -1;
    }
    else {
      if( karma::generate( at, karma::little_dword, value ) ) return;
      else throw -1;
    }
  }

  template< typename T > 
  void store_value_at(
    T value,
    uint8_t *at, bool is_big_endian,
    typename boost::enable_if<
      boost::mpl::or_<
        boost::is_same< typename boost::remove_cv< T >::type, int64_t >,
        boost::is_same< typename boost::remove_cv< T >::type, uint64_t >
      >
    >::type* = 0
  ) {
    namespace karma = boost::spirit::karma;
    if( is_big_endian ) {
      if( karma::generate( at, karma::big_qword, value ) ) return;
      else throw -1;
    }
    else {
      if( karma::generate( at, karma::little_qword, value ) ) return;
      else throw -1;
    }
  }

  template< typename T > 
  void store_value_at(
    T value,
    uint8_t *at, bool is_big_endian,
    typename boost::enable_if<
      boost::is_same< typename boost::remove_cv< T >::type, float >
    >::type* = 0
  ) {
    namespace karma = boost::spirit::karma;
    if( is_big_endian ) {
      if( karma::generate( at, karma::big_bin_float, value ) ) return;
      else throw -1;
    }
    else {
      if( karma::generate( at, karma::little_bin_float, value ) ) return;
      else throw -1;
    }
  }

  template< typename T > 
  void store_value_at(
    T value,
    uint8_t *at, bool is_big_endian,
    typename boost::enable_if<
      boost::is_same< typename boost::remove_cv< T >::type, double >
    >::type* = 0
  ) {
    namespace karma = boost::spirit::karma;
    if( is_big_endian ) {
      if( karma::generate( at, karma::big_bin_double, value ) ) return;
      else throw -1;
    }
    else {
      if( karma::generate( at, karma::little_bin_double, value ) ) return;
      else throw -1;
    }
  }
  }
}

#endif
