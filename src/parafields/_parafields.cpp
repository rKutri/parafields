#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dune/common/fvector.hh>

#include <parafields/randomfield.hh>

#include <memory>
#include <string>

namespace py = pybind11;

namespace parafields {

template<typename DF, typename RF, unsigned int dimension>
class GridTraits
{
public:
  enum
  {
    dim = dimension
  };

  using RangeField = RF;
  using Scalar = Dune::FieldVector<RF, 1>;
  using DomainField = DF;
  using Domain = Dune::FieldVector<DF, dim>;
};

#define GENERATE_FIELD_DIM(dim, t)                                             \
  using RandomField##dim##D_##t =                                              \
    Dune::RandomField::RandomField<GridTraits<t, t, dim>>;                     \
  py::class_<RandomField##dim##D_##t> field##dim##d_##t(                       \
    m, "RandomField" #dim "D_" #t);                                            \
  field##dim##d_##t.def(py::init<Dune::ParameterTree>());                      \
  field##dim##d_##t.def("generate",                                            \
                        [](RandomField##dim##D_##t& self, unsigned int seed) { \
                          self.generate(seed);                                 \
                        });                                                    \
  field##dim##d_##t.def(                                                       \
    "probe",                                                                   \
    [](const RandomField##dim##D_##t& self, const py::array_t<t>& pos) {       \
      Dune::FieldVector<t, 1> out;                                             \
      Dune::FieldVector<t, dim> apos;                                          \
      std::copy(pos.data(), pos.data() + dim, apos.begin());                   \
      self.evaluate(apos, out);                                                \
      return out[0];                                                           \
    });                                                                        \
                                                                               \
  field##dim##d_##t.def("eval", [](const RandomField##dim##D_##t& self) {      \
    std::array<unsigned int, dim> size;                                        \
    std::vector<Dune::FieldVector<t, 1>> out;                                  \
    self.bulkEvaluate(out, size);                                              \
    std::array<std::size_t, dim> strides;                                      \
    strides[0] = sizeof(t);                                                    \
    for (std::size_t i = 1; i < dim; ++i)                                      \
      strides[i] = strides[i - 1] * size[i - 1];                               \
    return py::array(py::dtype::of<t>(), size, strides, out[0].data());        \
  });

PYBIND11_MODULE(_parafields, m)
{
  m.doc() =
    "The parafields Python package for (parallel) parameter field generation";

  // Expose the Dune::ParameterTree class to allow the Python side to
  // easily feed data into the C++ code.
  py::class_<Dune::ParameterTree> param(m, "ParameterTree");
  param.def(py::init<>());
  param.def("set",
            [](Dune::ParameterTree& self, std::string key, std::string value) {
              self[key] = value;
            });

  // Expose the RandomField template instantiations

  // Double Precision for dim=1, 2, 3
#ifdef HAVE_FFTW3_DOUBLE
  GENERATE_FIELD_DIM(1, double)
  GENERATE_FIELD_DIM(2, double)
  GENERATE_FIELD_DIM(3, double)
#endif

  // Single Precision for dim=1, 2, 3
#ifdef HAVE_FFTW3_FLOAT
  GENERATE_FIELD_DIM(1, float)
  GENERATE_FIELD_DIM(2, float)
  GENERATE_FIELD_DIM(3, float)
#endif

  // Export to Python whether we built this against FakeMPI
  m.def("uses_fakempi", []() {
#if MPI_IS_FAKEMPI
    return true;
#else
    return false;
#endif
  });

  // Export the available floating point precision
  m.def("has_precision", [](std::string type) {
#ifdef HAVE_FFTW3_DOUBLE
    if (type == "double")
      return true;
#endif
#ifdef HAVE_FFTW3_FLOAT
    if (type == "float")
      return true;
#endif
    return false;
  });
}

} // namespace parafields
