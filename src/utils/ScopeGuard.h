#pragma once

namespace utils {
template <typename F>
class ScopeGuard {
 private:
  F _func;

 public:
  explicit ScopeGuard(F func) : _func(func) {}

  ~ScopeGuard() { _func(); }

  ScopeGuard(const ScopeGuard<F>&) = delete;
  ScopeGuard<F>& operator=(const ScopeGuard<F>&) = delete;
  ScopeGuard(ScopeGuard<F>&&) = delete;
  ScopeGuard<F>& operator=(ScopeGuard<F>&&) = delete;
};
}  // namespace utils