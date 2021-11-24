// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jun  6 12:58:17 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <cstring>
#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <sstream>

// system:
#ifndef _WIN32
#include <bfd.h>
#define HAVE_DECL_BASENAME 1
#include <demangle.h>
#endif

// boost library:
#include <boost/chrono/chrono.hpp>
#include <boost/thread.hpp>

// aeyae:
#include "yae/api/yae_assert.h"
#include "yae/utils/yae_benchmark.h"
#include "yae/utils/yae_utils.h"


//----------------------------------------------------------------
// indent
//
// FIXME: this probably exists somewhere in the codebase already
//
static std::string
indent(const char * text, std::size_t depth)
{
  static const char * whitespace =
    "                                                                ";
  static const std::size_t whitespace_max = std::strlen(whitespace);

  std::ostringstream os;
  for (std::size_t i = 0, n = depth / whitespace_max; i < n; i++)
  {
    os << whitespace;
  }

  os << &whitespace[whitespace_max - depth % whitespace_max];
  os << text;

  return std::string(os.str().c_str());
}

namespace yae
{

  //----------------------------------------------------------------
  // TBenchmark::Private
  //
  struct TBenchmark::Private
  {
    Private(const char * description);
    ~Private();

    static void show(std::ostream & os);
    static void clear();

    //----------------------------------------------------------------
    // Timesheet
    //
    struct Timesheet
    {
      Timesheet():
        t0_(boost::chrono::steady_clock::now()),
        depth_(0),
        path_("/")
      {}

      //----------------------------------------------------------------
      // Entry
      //
      struct Entry
      {
        Entry(std::size_t depth = 0,
              const std::string & path = std::string("/"),
              const char * name = ""):
          depth_(depth),
          path_(path),
          name_(name),
          n_(0),
          t_(0),
          fresh_(true)
        {
          YAE_ASSERT(*name);
        }

        std::size_t depth_;
        std::string path_;
        std::string name_;
        uint64 n_; // total number of occurrances
        uint64 t_; // total time spent, measured in microseconds
        bool fresh_;
      };

      boost::chrono::steady_clock::time_point t0_;
      std::map<std::string, Entry> entries_;
      std::size_t depth_;
      std::string path_;
    };

  protected:
    static boost::mutex mutex_;

    // thread-specific timesheets
    static std::map<boost::thread::id, Timesheet> tss_;

    std::string key_;
    boost::chrono::steady_clock::time_point t0_;
  };

  //----------------------------------------------------------------
  // TBenchmark::Private::mutex_
  //
  boost::mutex TBenchmark::Private::mutex_;

  //----------------------------------------------------------------
  // TBenchmark::Private::tss_
  //
  std::map<boost::thread::id, TBenchmark::Private::Timesheet>
  TBenchmark::Private::tss_;


  //----------------------------------------------------------------
  // TBenchmark::Private::Private
  //
  TBenchmark::Private::Private(const char * description)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    // lookup the timesheet for the current thread:
    boost::thread::id threadId = boost::this_thread::get_id();
    Timesheet & ts = tss_[threadId];

    // shortcut to timesheet entries:
    std::map<std::string, Timesheet::Entry> & entries = ts.entries_;

    std::string entryPath = ts.path_;
    ts.path_ += '/';
    ts.path_ += description;
    key_ = ts.path_;

    std::map<std::string, Timesheet::Entry>::iterator
      lower_bound = entries.lower_bound(key_);

    if (lower_bound == entries.end() ||
        entries.key_comp()(key_, lower_bound->first))
    {
      // initialize a new measurement
      Timesheet::Entry newEntry(ts.depth_, entryPath, description);
      entries.insert(lower_bound, std::make_pair(key_, newEntry));
    }
    else
    {
      lower_bound->second.fresh_ = true;
    }

    ts.depth_++;

    // save timestamp last, so that the benchmark would
    // not be older than the timesheet that keeps its entry:
    t0_ = boost::chrono::steady_clock::now();
  }

  //----------------------------------------------------------------
  // TBenchmark::Private::~Private
  //
  TBenchmark::Private::~Private()
  {
    boost::chrono::steady_clock::time_point
      t1 = boost::chrono::steady_clock::now();

    uint64 dt =
      boost::chrono::duration_cast<boost::chrono::microseconds>(t1 - t0_).
      count();

    boost::lock_guard<boost::mutex> lock(mutex_);

    // lookup the timesheet for the current thread:
    boost::thread::id threadId = boost::this_thread::get_id();
    Timesheet & ts = tss_[threadId];

    std::map<std::string, Timesheet::Entry>::iterator
      found = ts.entries_.find(key_);

    if (found == ts.entries_.end())
    {
      // this benchmark was created before timesheet was cleared, ignore it:
      return;
    }

    // lookup this benchmark timesheet entry:
    Timesheet::Entry & entry = found->second;
    YAE_ASSERT(entry.name_.size() && entry.name_[0]);

    entry.n_++;
    entry.t_ += dt;

    if (entry.fresh_)
    {
      YAE_ASSERT(ts.depth_ > 0);
      ts.depth_--;
      ts.path_ = entry.path_;
    }
  }

  //----------------------------------------------------------------
  // TBenchmark::Private::show
  //
  void
  TBenchmark::Private::show(std::ostream & os)
  {
    static const uint64 timebase = 1000000;
    boost::lock_guard<boost::mutex> lock(mutex_);

    std::ostringstream oss;
    oss <<  "\nBenchmark timesheets per thread:\n";

    for (std::map<boost::thread::id, Timesheet>::const_iterator
           j = tss_.begin(); j != tss_.end(); ++j)
    {
      boost::thread::id threadId = j->first;
      oss << "\n Thread " << threadId << " Timesheet:\n";

      // shortcuts:
      const Timesheet & ts = j->second;
      const std::map<std::string, Timesheet::Entry> & entries = ts.entries_;

      for (std::map<std::string, Timesheet::Entry>::const_iterator
             i = entries.begin(); i != entries.end(); ++i)
      {
        const Timesheet::Entry & entry = i->second;
        if (entry.n_ < 1)
        {
          continue;
        }

        oss
          << "  "
          << std::left << std::setw(40) << std::setfill(' ')
          << indent(entry.name_.c_str(), entry.depth_)
          << " : "

          << std::right << std::setw(8) << std::setfill(' ')
          << entry.n_
          << " "

          << std::right << std::setw(5) << std::setfill(' ')
          << (entry.n_ == 1 ? "call" : "calls")
          << ", "

          << std::fixed << std::setprecision(3) << std::setw(13)
          << double(entry.t_ * 1000) / double(timebase)
          << " msec total, "

          << std::fixed << std::setprecision(3) << std::setw(13)
          << double(entry.t_ * 1000000) / double(entry.n_ * timebase)
          << " usec avg\n";
      }
    }

    os << oss.str().c_str() << '\n';
  }

  //----------------------------------------------------------------
  // TBenchmark::Private::clear
  //
  void
  TBenchmark::Private::clear()
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    for (std::map<boost::thread::id, Timesheet>::iterator
           j = tss_.begin(), j1; j != tss_.end(); )
    {
      j1 = j; ++j1;

      // shortcuts:
      Timesheet & ts = j->second;
      std::map<std::string, Timesheet::Entry> & entries = ts.entries_;

      for (std::map<std::string, Timesheet::Entry>::iterator
             i = entries.begin(), i1; i != entries.end(); )
      {
        i1 = i; ++i1;

        Timesheet::Entry & entry = i->second;
        if (entry.n_ > 0)
        {
          entries.erase(i);
        }
        else
        {
          entry.fresh_ = false;
        }

        i = i1;
      }

      if (entries.empty())
      {
        tss_.erase(j);
      }
      else
      {
        ts.depth_ = 0;
        ts.path_ = "/";
        ts.t0_ = boost::chrono::steady_clock::now();
      }

      j = j1;
    }
  }


  //----------------------------------------------------------------
  // TBenchmark::TBenchmark
  //
  TBenchmark::TBenchmark(const char * description):
    private_(new TBenchmark::Private(description))
  {}

  //----------------------------------------------------------------
  // TBenchmark::~TBenchmark
  //
  TBenchmark::~TBenchmark()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // TBenchmark::show
  //
  void
  TBenchmark::show(std::ostream & os)
  {
    Private::show(os);
  }

  //----------------------------------------------------------------
  // TBenchmark::clear
  //
  void
  TBenchmark::clear()
  {
    Private::clear();
  }


  //----------------------------------------------------------------
  // TLifetime::Private
  //
  struct TLifetime::Private
  {
    ~Private();

    void start(const char * description);
    void finish();

    static void show(std::ostream & os);
    static void clear();

    //----------------------------------------------------------------
    // Timesheet
    //
    struct Timesheet
    {
      //----------------------------------------------------------------
      // Entry
      //
      struct Entry
      {
        Entry():
          n_(0),
          t_(0)
        {}

        uint64 n_; // total number of occurrances
        uint64 t_; // total time spent, measured in microseconds
      };

      Timesheet():
        t0_(boost::chrono::steady_clock::now())
      {}

      boost::chrono::steady_clock::time_point t0_;
      std::map<std::string, Entry> entries_;
    };

  protected:
    static boost::mutex mutex_;

    // since lifetime may start on one thread and end on another
    // all lifetimes are tracked on one timesheet:
    static Timesheet tss_;

    std::string key_;
    boost::chrono::steady_clock::time_point t0_;
  };

  //----------------------------------------------------------------
  // TLifetime::Private::mutex_
  //
  boost::mutex TLifetime::Private::mutex_;

  //----------------------------------------------------------------
  // TLifetime::Private::tss_
  //
  TLifetime::Private::Timesheet TLifetime::Private::tss_;


  //----------------------------------------------------------------
  // TLifetime::Private::~Private
  //
  TLifetime::Private::~Private()
  {
    finish();
  }

  //----------------------------------------------------------------
  // TLifetime::Private::start
  //
  void
  TLifetime::Private::start(const char * description)
  {
    YAE_ASSERT(description && *description);

    t0_ = boost::chrono::steady_clock::now();
    key_.assign(description);
  }

  //----------------------------------------------------------------
  // TLifetime::Private::finish
  //
  void
  TLifetime::Private::finish()
  {
    YAE_ASSERT(key_.size() && key_[0]);

    boost::chrono::steady_clock::time_point
      t1 = boost::chrono::steady_clock::now();

    uint64 dt =
      boost::chrono::duration_cast<boost::chrono::microseconds>(t1 - t0_).
      count();

    // lookup/create timesheet entry for this lifetime:
    boost::lock_guard<boost::mutex> lock(mutex_);
    Timesheet::Entry & entry = tss_.entries_[key_];
    entry.n_++;
    entry.t_ += dt;
  }

  //----------------------------------------------------------------
  // TLifetime::Private::show
  //
  void
  TLifetime::Private::show(std::ostream & os)
  {
    static const uint64 timebase = 1000000;

    boost::lock_guard<boost::mutex> lock(mutex_);

    boost::chrono::steady_clock::time_point
      t_now = boost::chrono::steady_clock::now();

    uint64 dt_usec =
      boost::chrono::duration_cast<boost::chrono::microseconds>
      (t_now - tss_.t0_).count();

    std::ostringstream oss;
    oss <<  "\nLifetime timesheet over the last "
        << double(dt_usec) * 1e-6 << " seconds:\n";

    for (std::map<std::string, Timesheet::Entry>::const_iterator
           i = tss_.entries_.begin(); i != tss_.entries_.end(); ++i)
    {
      const std::string & key = i->first;
      const Timesheet::Entry & entry = i->second;

      oss
        << "  "
        << std::left << std::setw(40) << std::setfill(' ')
        << key.c_str()
        << " : "

        << std::right << std::setw(8) << std::setfill(' ')
        << entry.n_
        << ", "

        << std::fixed << std::setprecision(3) << std::setw(13)
        << double(entry.t_ * 1000) / double(timebase)
        << " msec total, "

        << std::fixed << std::setprecision(3) << std::setw(13)
        << double(entry.t_ * 1000000) / double(entry.n_ * timebase)
        << " usec avg\n";
    }

    os << oss.str().c_str() << '\n';
  }

  //----------------------------------------------------------------
  // TLifetime::Private::clear
  //
  void
  TLifetime::Private::clear()
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    tss_.t0_ = boost::chrono::steady_clock::now();
    tss_.entries_.clear();
  }


  //----------------------------------------------------------------
  // TLifetime::TLifetime
  //
  TLifetime::TLifetime():
    private_(new TLifetime::Private())
  {}

  //----------------------------------------------------------------
  // TLifetime::~TLifetime
  //
  TLifetime::~TLifetime()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // TLifetime::start
  //
  void
  TLifetime::start(const char * description)
  {
    if (private_)
    {
      private_->start(description);
    }
  }

  //----------------------------------------------------------------
  // TLifetime::finish
  //
  void
  TLifetime::finish()
  {
    delete private_;
    private_ = NULL;
  }

  //----------------------------------------------------------------
  // TLifetime::show
  //
  void
  TLifetime::show(std::ostream & os)
  {
    Private::show(os);
  }

  //----------------------------------------------------------------
  // TLifetime::clear
  //
  void
  TLifetime::clear()
  {
    Private::clear();
  }


  //----------------------------------------------------------------
  // operator <<
  //
  std::ostream &
  operator << (std::ostream & os, const StackFrame & f)
  {
    os << std::right << std::setw(18) << std::setfill(' ') << f.address_
       << ' ' << f.func_ << " + " << f.offset_;
    return os;
  }

  //----------------------------------------------------------------
  // demangle
  //
  void
  demangle(StackFrame & frame, const char * line)
  {
#ifdef __APPLE__
    std::istringstream iss(line);
    int index = std::numeric_limits<int>::min();
    std::string symbol;
    std::string plus;
    iss >> index
        >> frame.module_
        >> frame.address_
        >> frame.func_
        >> plus
        >> frame.offset_;

    int status = 0;
    char * name = abi::__cxa_demangle(frame.func_.c_str(),
                                      0, 0, &status);
    if (status == 0 && name)
    {
      frame.func_ = name;
      free(name);
    }
#elif !defined(_WIN32)
    // NOTE: it may be possible to convert symbol + offset
    // to a file line number, using libbfd -- https://en.wikibooks.org/wiki/
    // Linux_Applications_Debugging_Techniques/The_call_stack

    const char * module = line;
    const char * symbol = NULL;
    const char * offset = NULL;
    const char * address = NULL;

    for (const char * i = line; i && *i; ++i)
    {
      if (*i == '(')
      {
        if (module < i)
        {
          frame.module_.assign(module, i);
        }

        symbol = i + 1;
      }
      else if (*i == '+')
      {
        if (symbol && symbol < i)
        {
          frame.func_.assign(symbol, i);
          int status = 0;
          char * name = abi::__cxa_demangle(frame.func_.c_str(),
                                            0, 0, &status);
          if (status == 0 && name)
          {
            frame.func_ = name;
            free(name);
          }
        }

        offset = i + 1;
      }
      else if (*i == ')')
      {
        if (offset && offset < i)
        {
          frame.offset_.assign(offset, i);
        }
      }
      else if (*i == '[')
      {
        address = i + 1;
      }
      else if (*i == ']')
      {
        if (address && address < i)
        {
          frame.address_.assign(address, i);
        }
      }
    }
#endif
  }

#ifndef _WIN32

  //----------------------------------------------------------------
  // bfd_wrapper_t
  //
  // https://kernel.googlesource.com/pub/scm/linux/kernel/
  // git/hjl/binutils/+/hjl/secondary/binutils/addr2line.c
  //
  // https://en.wikibooks.org/wiki/Linux_Applications_Debugging_Techniques/
  // The_call_stack
  //
  struct bfd_wrapper_t
  {

    //----------------------------------------------------------------
    // bfd_wrapper_t
    //
    bfd_wrapper_t():
      abfd_(NULL),
      syms_(NULL)
    {
      static unsigned int bfd_init_result = bfd_init();
      YAE_ASSERT(bfd_init_result == BFD_INIT_MAGIC);
      YAE_THROW_IF(bfd_init_result != BFD_INIT_MAGIC);

      YAE_ASSERT(get_current_executable_path(exe_path_));
      YAE_THROW_IF(exe_path_.empty());

      abfd_ = bfd_openr(exe_path_.c_str(), 0);
      YAE_ASSERT(abfd_);
      YAE_THROW_IF(!abfd_);

      bfd_check_format(abfd_, bfd_object);

      // slurp_symtab:
      if ((bfd_get_file_flags(abfd_) & HAS_SYMS) == 0)
      {
        return;
      }

      bfd_boolean dynamic = FALSE;
      long storage = bfd_get_symtab_upper_bound(abfd_);
      if (storage == 0)
      {
        storage = bfd_get_dynamic_symtab_upper_bound(abfd_);
        dynamic = TRUE;
      }

      YAE_THROW_IF(storage < 0);
      syms_ = (asymbol **)::malloc(storage);

      long symcount = dynamic ?
        bfd_canonicalize_dynamic_symtab(abfd_, syms_) :
        bfd_canonicalize_symtab(abfd_, syms_);

      YAE_THROW_IF(symcount < 0);
    }

    //----------------------------------------------------------------
    // ~bfd_wrapper_t
    //
    ~bfd_wrapper_t()
    {
      if (abfd_)
      {
        bfd_close(abfd_);
        abfd_ = NULL;
      }

      if (syms_)
      {
        ::free(syms_);
        syms_ = NULL;
      }
    }

    //----------------------------------------------------------------
    // ctx_t
    //
    struct ctx_t
    {
      ctx_t(const bfd_wrapper_t * wrapper = NULL, const void * addr = NULL):
        wrapper_(wrapper),
        addr_((bfd_vma)(addr)),
        found_(false),
        file_(NULL),
        func_(NULL),
        line_(0),
        discriminator_(0)
      {}

      const bfd_wrapper_t * wrapper_;
      bfd_vma addr_;
      bool found_;

      const char * file_;
      const char * func_;
      unsigned int line_;
      unsigned int discriminator_;
      std::string demangled_;

      std::list<ctx_t> inlined_by_;
    };

    //----------------------------------------------------------------
    // find_address_in_section
    //
    static void
    find_address_in_section(bfd * abfd, asection * section, void * user_data)
    {
      ctx_t * ctx = (ctx_t *)user_data;
      ctx->wrapper_->search(abfd, section, *ctx);
    }

    //----------------------------------------------------------------
    // search
    //
    void
    search(bfd * abfd, asection * section, ctx_t & ctx) const
    {
      YAE_ASSERT(abfd == abfd_);

      if (ctx.found_)
      {
        return;
      }

#ifdef bfd_get_section_flags
      if ((bfd_get_section_flags(abfd, section) & SEC_ALLOC) == 0)
      {
        return;
      }
#else
      if ((bfd_section_flags(section) & SEC_ALLOC) == 0)
      {
        return;
      }
#endif

#ifdef bfd_get_section_vma
      bfd_vma vma = bfd_get_section_vma(abfd, section);
#else
      bfd_vma vma = bfd_section_vma(section);
#endif
      if (ctx.addr_ < vma)
      {
        return;
      }

#ifdef bfd_get_section_size
      bfd_size_type size = bfd_get_section_size(section);
#else
      bfd_size_type size = bfd_section_size(section);
#endif
      if (vma + size <= ctx.addr_)
      {
        return;
      }

      ctx.found_ = bfd_find_nearest_line_discriminator(abfd,
                                                       section,
                                                       syms_,
                                                       ctx.addr_ - vma,
                                                       &ctx.file_,
                                                       &ctx.func_,
                                                       &ctx.line_,
                                                       &ctx.discriminator_);
    }

    //----------------------------------------------------------------
    // resolve
    //
    bool resolve(const void * addr, ctx_t & ctx) const
    {
      ctx = ctx_t(this, addr);
#if 0
      if (bfd_get_flavour(abfd_) == bfd_target_elf_flavour)
      {
        const struct elf_backend_data * bed = get_elf_backend_data(abfd_);
        bfd_vma sign = (bfd_vma) 1ull << (bed->s->arch_size - 1);
        ctx.addr_ &= (sign << 1) - 1;

        if (bed->sign_extend_vma)
        {
          ctx.addr_ = (ctx.addr_ ^ sign) - sign;
        }
      }
#endif
      ctx.found_ = FALSE;
      bfd_map_over_sections(abfd_,
                            bfd_wrapper_t::find_address_in_section,
                            &ctx);

      ctx_t * resolved = &ctx;
      while (resolved->found_)
      {
        if (resolved->func_ && *resolved->func_)
        {
          char * alloc =
            bfd_demangle(abfd_, resolved->func_, DMGL_ANSI | DMGL_PARAMS);

          if (alloc != NULL)
          {
            resolved->demangled_ = alloc;
            free(alloc);
          }
        }

        resolved->inlined_by_.push_back((ctx_t()));
        ctx_t & inlined_by = resolved->inlined_by_.back();
        inlined_by.found_ = bfd_find_inliner_info(abfd_,
                                                  &inlined_by.file_,
                                                  &inlined_by.func_,
                                                  &inlined_by.line_);
        if (!inlined_by.found_)
        {
          resolved->inlined_by_.pop_back();
          break;
        }

        resolved = &inlined_by;
      }

      return ctx.found_;
    }

    std::string exe_path_;
    bfd * abfd_;
    asymbol ** syms_;
  };

  //----------------------------------------------------------------
  // bfd_wrapper
  //
  static const bfd_wrapper_t & bfd_wrapper()
  {
    static bfd_wrapper_t bfd_wrapper_;
    return bfd_wrapper_;
  }
#endif

  //----------------------------------------------------------------
  // StackTrace::Private
  //
  struct StackTrace::Private
  {
    std::vector<void *> frames_;

    Private()
    {
#ifndef _WIN32
      frames_.resize(256);
      std::size_t num_frames = ::backtrace(&(frames_[0]), frames_.size());
      frames_.resize(num_frames);
#endif
    }

    std::string to_str(std::size_t offset, const char * sep = "\n") const
    {
#ifndef _WIN32
      std::ostringstream oss;

      const bfd_wrapper_t & abfd = bfd_wrapper();
      for (std::size_t i = offset, n = frames_.size(); i < n; i++)
      {
        const void * addr = frames_[i];
        oss << std::setw(14) << addr;

        const char * file = NULL;
        const char * func = NULL;
        unsigned int line = std::numeric_limits<unsigned int>::max();

        bfd_wrapper_t::ctx_t ctx;
        if (abfd.resolve(frames_[i], ctx))
        {
          oss << " " << ctx.file_ << ":" << ctx.line_;
          if (!ctx.demangled_.empty())
          {
            oss << " " << ctx.demangled_;
          }
          else if (ctx.func_ && *ctx.func_)
          {
            oss << " "<< ctx.func_;
          }

          for (std::list<bfd_wrapper_t::ctx_t>::const_iterator
                 j = ctx.inlined_by_.begin(); j != ctx.inlined_by_.end(); ++j)
          {
            const bfd_wrapper_t::ctx_t & inlined_by = *j;
            oss << ", inlined by "
                << inlined_by.file_ << ":" << inlined_by.line_;

            if (!inlined_by.demangled_.empty())
            {
              oss << " " << inlined_by.demangled_;
            }
            else if (inlined_by.func_ && *inlined_by.func_)
            {
              oss << " "<< inlined_by.func_;
            }
          }
        }
        oss << sep;
      }

      return oss.str();
#endif
    }
  };

  //----------------------------------------------------------------
  // StackTrace::StackTrace
  //
  StackTrace::StackTrace()
  {
#ifndef _WIN32
    private_ = new StackTrace::Private();
#endif
  }

  //----------------------------------------------------------------
  // StackTrace::StackTrace
  //
  StackTrace::StackTrace(const StackTrace & bt):
    private_(bt.private_ ? new StackTrace::Private(*bt.private_) : NULL)
  {}

  //----------------------------------------------------------------
  // StackTrace::~StackTrace
  //
  StackTrace::~StackTrace()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // StackTrace::operator
  //
  StackTrace &
  StackTrace::operator = (const StackTrace & bt)
  {
    if (this != &bt)
    {
      delete private_;
      private_ = bt.private_ ? new StackTrace::Private(*bt.private_) : NULL;
    }

    return *this;
  }

  //----------------------------------------------------------------
  // StackTrace::to_str
  //
  std::string
  StackTrace::to_str(std::size_t offset, const char * sep) const
  {
    return private_ ? private_->to_str(offset, sep) : std::string();
  }

  //----------------------------------------------------------------
  // capture_backtrace
  //
  void
  capture_backtrace(std::list<StackFrame> & bt, std::size_t offset)
  {
#ifndef _WIN32
    void * frames[100] = { NULL };
    std::size_t num_frames = backtrace(frames, sizeof(frames) / sizeof(void *));
    char ** symbols = backtrace_symbols(frames, num_frames);

    for (std::size_t i = offset; i < num_frames; i++)
    {
      bt.push_back(StackFrame());
      StackFrame & f = bt.back();
      demangle(f, symbols[i]);
    }

    free(symbols);
#endif
  }

  //----------------------------------------------------------------
  // dump
  //
  std::ostream &
  dump(std::ostream & os, const std::list<StackFrame> & backtrace)
  {
    for (std::list<StackFrame>::const_iterator i = backtrace.begin();
         i != backtrace.end(); ++i)
    {
      const StackFrame & f = *i;
      os << f << "\n";
    }

    return os;
  }

  //----------------------------------------------------------------
  // dump_stacktrace
  //
  std::ostream &
  dump_stacktrace(std::ostream & os, std::size_t offset)
  {
    std::list<StackFrame> backtrace;
    capture_backtrace(backtrace, offset);
    dump(os, backtrace);
    return os;
  }

  //----------------------------------------------------------------
  // get_stacktrace_str
  //
  std::string
  get_stacktrace_str(std::size_t offset)
  {
#if 0
    std::list<StackFrame> backtrace;
    capture_backtrace(backtrace, offset);

    std::ostringstream oss;
    dump(oss, backtrace);

    std::string bt(oss.str().c_str());
    return bt;
#else
    StackTrace bt;
    return bt.to_str(offset);
#endif
  }

#ifdef YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS
  //----------------------------------------------------------------
  // TFootprint::Private
  //
  struct TFootprint::Private
  {
    Private(const char * name, std::size_t size);
    ~Private();

    inline const std::string & name() const
    { return name_; }

    void capture_backtrace();

    static void show(std::ostream & os);
    static void clear();

    //----------------------------------------------------------------
    // Entry
    //
    struct Entry
    {
      Entry(const std::string & name = std::string()):
        name_(name),
        n_(0),
        z_(0)
      {}

      std::string name_;
      uint64 n_; // total number of occurrances
      uint64 z_; // total footprint, measured in bytes
      std::set<const Private *> p_;
    };

  protected:
    static boost::mutex mutex_;
    static std::map<std::string, Entry> entries_;

    std::string name_;
    std::size_t size_;
    std::list<std::string> bt_;
  };

  //----------------------------------------------------------------
  // TFootprint::Private::mutex_
  //
  boost::mutex TFootprint::Private::mutex_;

  //----------------------------------------------------------------
  // TFootprint::Private::entries_
  //
  std::map<std::string, TFootprint::Private::Entry>
  TFootprint::Private::entries_;


  //----------------------------------------------------------------
  // TFootprint::Private::Private
  //
  TFootprint::Private::Private(const char * name, std::size_t size)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    name_ = name;
    size_ = size;

    std::map<std::string, Entry>::iterator found = entries_.lower_bound(name_);

    if (found == entries_.end() ||
        entries_.key_comp()(name_, found->first))
    {
      // initialize a new measurement
      Entry entry(name_);
      found = entries_.insert(found, std::make_pair(name_, entry));
    }

    Entry & entry = found->second;
    entry.z_ += size;
    entry.n_ += 1;

    std::pair<std::set<const Private *>::iterator, bool>
      inserted = entry.p_.insert(this);
    YAE_ASSERT(inserted.second);
  }

  //----------------------------------------------------------------
  // TFootprint::Private::~Private
  //
  TFootprint::Private::~Private()
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    std::map<std::string, Entry>::iterator found = entries_.find(name_);

    if (found == entries_.end())
    {
      // this Footprint was created before timesheet was cleared, ignore it:
      return;
    }

    // lookup this Footprint timesheet entry:
    Entry & entry = found->second;
    YAE_ASSERT(entry.name_.size() && entry.name_[0] && entry.n_ > 0);

    entry.n_--;
    entry.z_ -= size_;

    std::size_t n = entry.p_.erase(this);
    YAE_ASSERT(n == 1);
  }

  //----------------------------------------------------------------
  // TFootprint::Private::capture_backtrace
  //
  void
  TFootprint::Private::capture_backtrace()
  {
    bt_.push_back(yae::get_stacktrace_str());
  }

  //----------------------------------------------------------------
  // TFootprint::Private::show
  //
  void
  TFootprint::Private::show(std::ostream & os)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    std::ostringstream oss;
    oss <<  "\nFootprints:\n";

    uint64_t total = 0;
    for (std::map<std::string, Entry>::const_iterator
           i = entries_.begin(); i != entries_.end(); ++i)
    {
      const Entry & entry = i->second;
      if (entry.n_ < 1)
      {
        continue;
      }

      total += entry.z_;

      oss
        << std::right << std::setw(9) << std::setfill(' ')
        << entry.n_
        << " * "
        << std::left << std::setw(4) << std::setfill(' ')
        << (entry.z_ / entry.n_)
        << " = "
        << std::left << std::setw(13)
        << entry.z_

        << " : "
        << std::left << std::setw(40) << std::setfill(' ')
        << entry.name_
        << "\n";

      const std::set<const Private *> & footprints = entry.p_;
      if (footprints.empty())
      {
        continue;
      }

      for (std::set<const Private *>::const_iterator
             y = footprints.begin(); y != footprints.end(); ++y)
      {
        const Private * footprint = *y;
        const std::list<std::string> & bt = footprint->bt_;

        if (bt.empty())
        {
          continue;
        }

        oss << "\n-------------------------------------------------------------"
            << "\n" << entry.name_ << "\n";

        for (std::list<std::string>::const_iterator
               z = bt.begin(); z != bt.end(); ++z)
        {
          oss << *y << "\n";
        }

        oss << "\n";
      }
    }

    oss << "Total footprint: " << total << "\n";

    os << oss.str().c_str() << '\n';
  }

  //----------------------------------------------------------------
  // TFootprint::Private::clear
  //
  void
  TFootprint::Private::clear()
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    for (std::map<std::string, Entry>::iterator
           i = entries_.begin(), i1; i != entries_.end(); )
    {
      i1 = i; ++i1;

      entries_.erase(i);
      i = i1;
    }
  }


  //----------------------------------------------------------------
  // TFootprint::TFootprint
  //
  TFootprint::TFootprint(const char * name, std::size_t size):
    private_(new TFootprint::Private(name, size))
  {}

  //----------------------------------------------------------------
  // TFootprint::~TFootprint
  //
  TFootprint::~TFootprint()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // TFootprint::name
  //
  const std::string &
  TFootprint::name() const
  {
    return private_->name();
  }

  //----------------------------------------------------------------
  // TFootprint::capture_backtrace
  //
  void
  TFootprint::capture_backtrace()
  {
    private_->capture_backtrace();
  }

  //----------------------------------------------------------------
  // TFootprint::show
  //
  void
  TFootprint::show(std::ostream & os)
  {
    Private::show(os);
  }

  //----------------------------------------------------------------
  // TFootprint::clear
  //
  void
  TFootprint::clear()
  {
    Private::clear();
  }
#endif // YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS

}
