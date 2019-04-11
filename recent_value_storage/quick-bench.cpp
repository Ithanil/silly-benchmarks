#include <vector>
#include <deque>
#include <list>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>

// --- Buffer Class

template <class ValueT>
class PushBackBuffer
{
protected:
    std::vector<ValueT> _vec; // the vector for actually storing the elements
    size_t _ncap; // the buffer capacity (max number of elements)
    size_t _inext; // the next internal storage index to be written at on push

    void _inc_inext() noexcept;
    size_t _get_index(size_t i) const noexcept;

public:
    explicit PushBackBuffer(size_t size = 0) noexcept;

    size_t size() const noexcept { return _vec.size(); }
    bool full() const noexcept { return (_vec.size() == _ncap); }
    bool wrapped() const noexcept { return (_inext < _vec.size()); } // is equivalent to full when cap > 0

    const ValueT &operator[](size_t i) const;

    void push_back(const ValueT &val);
    void push_back(ValueT &&val);
};

template <class ValueT>
PushBackBuffer<ValueT>::PushBackBuffer(const size_t size) noexcept
{
    _vec.reserve(size);
    _ncap = size;
    _inext = 0;
}

template <class ValueT>
void PushBackBuffer<ValueT>::_inc_inext() noexcept
{
    if (++_inext == _ncap) { _inext = 0; } // this beats modulo and is safe here
}

template <class ValueT>
size_t PushBackBuffer<ValueT>::_get_index(const size_t index) const noexcept
{   // assuming index < ncap
    if (this->wrapped()) {
      const size_t shiftidx = _inext + index;
      return (shiftidx < _ncap) ? shiftidx : shiftidx - _ncap; 
    }
    else {
      return index;
    }
}

template <class ValueT>
const ValueT &PushBackBuffer<ValueT>::operator[](const size_t i) const
{
    return _vec[this->_get_index(i)];
}

/* MODULO version (9x slower!)
template <class ValueT>
const ValueT &PushBackBuffer<ValueT>::operator[](const size_t i) const
{
    return (_inext < _vec.size()) ? _vec[(_inext + i) % _ncap] : _vec[i];
}
*/

template <class ValueT>
void PushBackBuffer<ValueT>::push_back(const ValueT &val)
{
    if (this->full()) {
        _vec[_inext] = val;
    }
    else {
        _vec.push_back(val);
    }
    this->_inc_inext();
}

template <class ValueT>
void PushBackBuffer<ValueT>::push_back(ValueT &&val)
{
    if (this->full()) {
        _vec[_inext] = val;
    }
    else {
        _vec.push_back(val);
    }
    this->_inc_inext();
}


// --- BENCHMARK

#define WHICH_BENCH 2 /*1->access benchmark, 2->push_back benchmark*/


#if WHICH_BENCH == 1
// --- Index Access benchmark

// Function to do something with container which supports []
template <class BufT>
auto foo(const BufT &buf, size_t i)
{
  auto ret = buf[0];
  for (size_t i=1; i<buf.size(); ++i) {
    ret += buf[i];
  }
  return ret;
}

// specialization for list (doesn't support [])
template <class ValueT>
auto foo(const std::list<ValueT> &lst, size_t i)
{
  auto ret = lst.front();
  for (auto it = ++lst.begin(); it != lst.end(); ++it){
    ret += *it;
  }
  return ret;
}

const size_t nbuf = 10000;

static void bench_access_buf(benchmark::State& state) {
  PushBackBuffer<double> buffer(nbuf);
  for (size_t i=0; i<nbuf+nbuf/2; ++i) { buffer.push_back(static_cast<double>(i)); } // overfill it a bit (i.e. make it wrap)

  size_t i=0;
  double s = 0.;
  for (auto _ : state) {
    s += 0.49*foo(buffer, i);
    if (++i == nbuf) { i = 0; }
  }
  std::cout << s << std::endl;
}
BENCHMARK(bench_access_buf);

static void bench_access_vec(benchmark::State& state) {
  std::vector<double> vec(nbuf); // to test buffer against raw vec
  std::iota(vec.begin(), vec.end(), 1. + nbuf/2); // fill similar stuff in, for the sake of it

  size_t i=0;
  double s = 0.;
  for (auto _ : state) {
    s += 0.49*foo(vec, i);
    if (++i == nbuf) { i = 0; }
  }
  std::cout << s << std::endl;
}
BENCHMARK(bench_access_vec);

static void bench_access_deque(benchmark::State& state) {
  std::deque<double> deq(nbuf);
  std::iota(deq.begin(), deq.end(), 1. + nbuf/2);

  size_t i=0;
  double s = 0.;
  for (auto _ : state) {
    s += 0.49*foo(deq, i);
    if (++i == nbuf) { i = 0; }
  }
  std::cout << s << std::endl;
}
BENCHMARK(bench_access_deque);

static void bench_access_list(benchmark::State& state) {
  std::list<double> lst(nbuf);
  std::iota(lst.begin(), lst.end(), 1. + nbuf/2);

  size_t i=0;
  double s = 0.;
  for (auto _ : state) {
    s += 0.49*foo(lst, i);
    if (++i == nbuf) { i = 0; }
  }
  std::cout << s << std::endl;
}
BENCHMARK(bench_access_list);


#elif WHICH_BENCH == 2

// Benchmarks to compare raw vector push_back vs. PushBackBuffer push_back (on average, no difference)

// Benchmark Struct (test element for buffer)
struct TestS
{
  struct member1
  {
    double x;
    double y;
  };
  struct member2
  {
    std::vector<double> X;
    std::vector<double> Y;
  };

  member1 A;
  member2 B;
 
  TestS() = default;

  TestS(size_t N)
  {
    A.x = 0.;
    A.y = 1.;
    B.X = std::vector<double>(N);
    B.Y = std::vector<double>(N);
    std::iota(B.X.begin(), B.X.end(), 0.);
    std::iota(B.Y.begin(), B.Y.end(), 0.);
  }
};

const size_t nbuf = 100;
const size_t ndim = 100;
const size_t ndim_s = (ndim-2)/2; // for TestS vectors

static void bench_pushb_buf(benchmark::State& state) {
  TestS test(ndim_s);
  PushBackBuffer<TestS> buffer(nbuf);

  for (auto _ : state) {
    buffer.push_back(test);
  }
}
BENCHMARK(bench_pushb_buf);


static void bench_pushb_vec(benchmark::State& state) {
  TestS test(ndim_s);
  std::vector<TestS> vec; // test buffer against raw vec (with push/erase)
  vec.reserve(nbuf);

  size_t i = 0;
  for (auto _ : state) {
    vec.push_back(test);
    if (i>=nbuf) { vec.erase(vec.begin()); }
    ++i;
  }
}
BENCHMARK(bench_pushb_vec);

static void bench_pushb_deque(benchmark::State& state) {
  TestS test(ndim_s);
  std::deque<TestS> deq; // test buffer against deque (with push/pop)

  size_t i = 0;
  for (auto _ : state) {
    deq.push_back(test);
    if (i>=nbuf) { deq.pop_front(); }
    ++i;
  }
}
BENCHMARK(bench_pushb_deque);

static void bench_pushb_list(benchmark::State& state) {
  TestS test(ndim_s);
  std::list<TestS> lst; // test buffer against list (with push/pop)

  size_t i = 0;
  for (auto _ : state) {
    lst.push_back(test);
    if (i>=nbuf) { lst.pop_front(); }
    ++i;
  }
}
BENCHMARK(bench_pushb_list);

#endif
