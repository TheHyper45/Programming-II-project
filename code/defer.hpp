#ifndef DEFER_HPP
#define DEFER_HPP

#include <utility>

#define CORE_CONCAT2_(X,Y) X##Y
#define CORE_CONCAT2(X,Y) CORE_CONCAT2_(X,Y)

/*	This class emulates 'defer' from other programming languages like Go.
	It allows us to use C-styled API that require us to manually free/destroy objects. */

template<typename Lambda>
class Deferred_Lambda {
public:
	Deferred_Lambda(Lambda&& _lambda) : lambda(std::move(_lambda)) {}
	~Deferred_Lambda() { lambda(); }
private:
	Lambda lambda;
};
#define defer Deferred_Lambda CORE_CONCAT2(_lambda,__LINE__) =

#endif
