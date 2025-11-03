#pragma once

#include "path.h"
#include "string.h"

#include <algorithm>
#include <ranges>
#include <vector>

namespace ole {

constexpr std::uint32_t NOSTREAM = 0xFFFFFFFF;

enum class file_type : signed char {
  unknown_or_unallocated   = 0,
  not_found = -1,
  directory = 1,
  regular   = 2,
  root   = 5
};

struct DirectoryEntry {
  file_type type;
  String name;
  std::uint32_t left_id;
  std::uint32_t right_id;
  std::uint32_t child_id;

  std::uint32_t starting_sector;
  std::uint64_t creation_time;
  std::uint64_t modified_time;
  std::uint64_t stream_size;
};

inline bool has_children(const DirectoryEntry& entry) noexcept { return entry.child_id != NOSTREAM; }

template<std::ranges::random_access_range R>
void inorder_impl(R dirs, std::vector<std::size_t>& result, std::size_t id) {
  if (id == NOSTREAM)
    return;
  inorder_impl(dirs, result, dirs[id].left_id);
  result.push_back(id);
  inorder_impl(dirs, result, dirs[id].right_id);
}

template<std::ranges::random_access_range R>
std::vector<std::size_t> inorder(R dirs, std::size_t id) {
  std::vector<std::size_t> result;
  inorder_impl(dirs, result, id);
  return result;
}

class InorderDirectoryIterator {
public:
  // Требования std::input_iterator
  using iterator_category = std::input_iterator_tag;
  using iterator_concept = std::input_iterator_tag;
  using reference = const DirectoryEntry&;
  using value_type       = DirectoryEntry;
  using difference_type  = std::ptrdiff_t;

  InorderDirectoryIterator(const std::vector<DirectoryEntry>& base, std::size_t root): base_(std::addressof(base)), q(inorder(base, root)) {}

  reference operator*() const { return dereference(); }
  const value_type* operator->() const { return std::addressof(dereference()); }

  InorderDirectoryIterator& operator++() {
    if (q.empty()) return *this;

    q.pop_back();
    return *this;
  }

  InorderDirectoryIterator operator++(int) {
    auto tmp = *this;
    ++*this;
    return tmp;
  }

  bool operator==([[maybe_unused]] std::default_sentinel_t unused) const { return q.empty(); }

private:
  [[nodiscard]] reference dereference() const& { return (*base_)[q.back()]; }

private:
  const std::vector<DirectoryEntry>* base_;
  std::vector<std::size_t> q;
};


template<std::ranges::viewable_range R>
class TraversalDirectoryIterator {
public:
  // Требования std::input_iterator
  using iterator_category = std::input_iterator_tag;
  using iterator_concept = std::input_iterator_tag;
  using reference = std::ranges::range_reference_t<R>;
  using value_type       = std::ranges::range_value_t<R>;
  using difference_type  = std::ranges::range_difference_t<R>;

  TraversalDirectoryIterator() = default;
  TraversalDirectoryIterator(R base, String key, std::size_t pos): base_(base), key_(std::move(key)), cur_(pos) {}

  reference operator*() const { return dereference(); }
  const value_type* operator->() const { return std::addressof(dereference()); }

  TraversalDirectoryIterator& operator++() {
    while (cur_ != NOSTREAM) {
      if (const auto& e = base_[cur_]; key_ < e.name) {
        cur_ = e.left_id;
      } else if (key_ > e.name) {
        cur_ = e.right_id;
      } else { break; }
    }

    return *this;
  }

  TraversalDirectoryIterator operator++(int) {
    auto tmp = *this;
    ++*this;
    return tmp;
  }

  bool operator==([[maybe_unused]] std::default_sentinel_t unused) const { return cur_ == NOSTREAM; }

private:
  [[nodiscard]] reference dereference() const& { return base_[cur_]; }

private:
  R base_;
  String key_;
  std::size_t cur_ = NOSTREAM;
};

class InorderDirectoryView final: public std::ranges::view_interface<InorderDirectoryView> {
public:
  InorderDirectoryView() = default;
  InorderDirectoryView(const std::vector<DirectoryEntry>& src, std::size_t root): base_(std::addressof(src)), root_(root) {}
  [[nodiscard]] InorderDirectoryIterator begin() const { return {*base_, root_}; }
  [[nodiscard]] std::default_sentinel_t end() const { return {}; }
private:
  const std::vector<DirectoryEntry>* base_;
  std::size_t root_ = NOSTREAM;
};

auto dir_view(const std::vector<DirectoryEntry>& base, std::size_t root) {
  return InorderDirectoryView(base, root);
}

template<std::ranges::viewable_range R>
class TraversalDirectoryView: public std::ranges::view_interface<TraversalDirectoryView<R>> {
public:
  TraversalDirectoryView() = default;
  TraversalDirectoryView(R base, String key, std::size_t pos): base_(base), key_(std::move(key)), pos_(pos) {}
  [[nodiscard]] TraversalDirectoryIterator<R> begin() const { return TraversalDirectoryIterator<R>(base_, key_, pos_); }
  [[nodiscard]] std::default_sentinel_t end() const { return {}; }
private:
  R base_;
  String key_;
  std::size_t pos_ = 0;
};

template<std::ranges::viewable_range R>
std::optional<std::ranges::range_value_t<R>> find(R base, String key, std::size_t pos) {
  auto view = TraversalDirectoryView(base, key, pos);
  auto b = cbegin(view);
  if (++b == cend(view)) {
    return std::nullopt;
  }

  return *b;
}

/*
template<std::ranges::viewable_range R>
class DirectoryLevelIterator {
public:
  // Требования std::input_iterator
  using iterator_category = std::forward_iterator_tag;
  using iterator_concept = std::forward_iterator_tag;
  using reference = InorderDirectoryView<R>;
  using value_type       = InorderDirectoryView<R>;
  using difference_type  = std::ptrdiff_t;

  DirectoryLevelIterator(R base, std::size_t root): base(base), root(root) {}

  reference operator*() const { return dir_view(base, root); }

  DirectoryLevelIterator& operator++() {
    auto dir = dir_view(base, root);
    auto found = std::ranges::find_if(dir, has_children);
    root = found == cend(dir) ? NOSTREAM : found->child_id;

    return *this;
  }

  DirectoryLevelIterator operator++(int) {
    auto tmp = *this;
    ++*this;
    return tmp;
  }

  bool operator==([[maybe_unused]] std::default_sentinel_t unused) const { return root == NOSTREAM; }
private:
  R base;
  std::size_t root = NOSTREAM;
};


template<std::ranges::viewable_range R>
class DirectoryLevelView: public std::ranges::view_interface<DirectoryLevelView<R>> {
private:
  using Base = R;
public:
  DirectoryLevelView() = default;
  DirectoryLevelView(R base, std::size_t root): base_(base), root_(root) {}
  [[nodiscard]] DirectoryLevelIterator<R> begin() const { return DirectoryLevelIterator<R>(base_, root_); }
  [[nodiscard]] std::default_sentinel_t end() const { return {}; }
private:
  Base base_;
  std::size_t root_ = NOSTREAM;
};

template<std::ranges::viewable_range R>
auto DirectoryLevels(R r, std::size_t root) {
  return DirectoryLevelView<R>(std::move(r), root);
}
*/
/**
 * Iterator that advances segment-by-segment, resolving each to an entry.
 */
class PathResolveIterator {
public:
  // Требования std::input_iterator
  using iterator_category = std::input_iterator_tag;
  using iterator_concept = std::input_iterator_tag;
  using reference = const DirectoryEntry&;
  using value_type       = DirectoryEntry;
  using difference_type  = std::ptrdiff_t;

  PathResolveIterator(
    const std::vector<DirectoryEntry>& base,
    std::vector<String>::const_iterator key,
    std::size_t root): base_(std::addressof(base)), key_(key), root_(root) {

    if (key_ == std::ranges::sentinel_t<Path> {}) {
      root_ = NOSTREAM;
    }

    while (root_ != NOSTREAM) {
      if (const auto& e = (*base_)[root_]; *key_ < e.name) {
        root_ = e.left_id;
      } else if (*key_ > e.name) {
        root_ = e.right_id;
      } else { break; }
    }
  }

  reference operator*() const { return dereference(); }
  const value_type* operator->() const { return std::addressof(dereference()); }

  PathResolveIterator& operator++() {
    *this = PathResolveIterator(*base_, next(key_), dereference().child_id);
    return *this;
  }

  PathResolveIterator operator++(int) {
    auto tmp = *this;
    ++*this;
    return tmp;
  }

  bool operator==([[maybe_unused]] std::default_sentinel_t unused) const { return root_ == NOSTREAM; }

private:
  [[nodiscard]] reference dereference() const& { return (*base_)[root_]; }

private:
  const std::vector<DirectoryEntry>* base_;
  std::vector<String>::const_iterator  key_;
  std::size_t root_ = NOSTREAM;
};

/**
 * A view that resolves a path into the sequence of matched DirectoryEntries.
 */
class PathResolveView: public std::ranges::view_interface<PathResolveView> {
public:
  PathResolveView() = default;
  PathResolveView(const std::vector<DirectoryEntry>& base, const ole::Path& target, std::size_t root): base_(std::addressof(base)), target_(std::addressof(target)), root_(root) {}
  [[nodiscard]] PathResolveIterator begin() const { return {*base_, std::cbegin(*target_), root_}; }
  [[nodiscard]] std::default_sentinel_t end() const { return {}; }
private:
  const std::vector<DirectoryEntry>* base_;
  const Path* target_;
  std::size_t root_ = NOSTREAM;
};

auto PathResolve(const std::vector<DirectoryEntry>& src, const ole::Path& target, std::size_t root) {
  return PathResolveView(src, target, root);
}

} // namespace ole