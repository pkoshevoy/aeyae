// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Tue Nov 23 19:28:41 MST 2021
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifdef __GNUC__
#define YAE_GCC_VERSION (__GNUC__ * 10000           \
                         + __GNUC_MINOR__ * 100     \
                         + __GNUC_PATCHLEVEL__)
#else
#define YAE_GCC_VERSION 0
#endif

// standard:
#include <iostream>
#include <iomanip>
#include <list>
#include <string>
#include <sstream>
#include <vector>

// system:
#ifdef YAE_HAS_LIBBFD
#include <bfd.h>
#ifndef HAVE_DECL_BASENAME
#define HAVE_DECL_BASENAME 1
#include <demangle.h>
#undef HAVE_DECL_BASENAME
#endif
#endif
#ifdef YAE_BACKTRACE_HEADER
#include <cxxabi.h>
#include <execinfo.h>
#endif

// aeyae:
#include "yae/api/yae_assert.h"
#include "yae/utils/yae_stacktrace.h"
#include "yae/utils/yae_utils.h"



namespace yae
{
  //----------------------------------------------------------------
  // StackFrame
  //
  struct StackFrame
  {
    std::string module_;
    std::string func_;
    std::string offset_;
    std::string address_;
  };

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


#if defined(_WIN32)
  //----------------------------------------------------------------
  // StackTraceWin32
  //
  // FIXME: write me!
  //
  struct StackTraceWin32
  {
    void capture(std::size_t max_frames = 256)
    {
      (void)max_frames;
      return;
    }

    std::string to_str(std::size_t offset, const char * sep = "\n") const
    {
      (void)offset;
      (void)sep;
      return std::string();
    }
  };

  //----------------------------------------------------------------
  // StackTrace::Private
  //
  struct StackTrace::Private : StackTraceWin32
  {};

#elif !defined(YAE_HAS_LIBBFD)
  //----------------------------------------------------------------
  // StackTraceApple
  //
  struct StackTraceApple
  {
    void capture(std::size_t max_frames = 256)
    {
#ifdef YAE_BACKTRACE_HEADER
      frames_.resize(max_frames);
      std::size_t num_frames = ::backtrace(&(frames_[0]), frames_.size());
      frames_.resize(num_frames);
#endif
    }

    std::string to_str(std::size_t offset, const char * sep = "\n") const
    {
      std::ostringstream oss;

#ifdef YAE_BACKTRACE_HEADER
      if (!frames_.empty())
      {
        char ** symbols = backtrace_symbols(&frames_[0], frames_.size());
        for (std::size_t i = offset, n = frames_.size(); i < n; i++)
        {
          StackFrame f;
          demangle(f, symbols[i]);
          oss << f << sep;
        }
        free(symbols);
      }
#endif
      return oss.str();
    }

    std::vector<void *> frames_;
  };

  //----------------------------------------------------------------
  // StackTrace::Private
  //
  struct StackTrace::Private : StackTraceApple
  {};

#else

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

  //----------------------------------------------------------------
  // StackTrace::Private
  //
  struct StackTraceLinux
  {
    void capture(std::size_t max_frames = 256)
    {
      frames_.resize(max_frames);
      std::size_t num_frames = ::backtrace(&(frames_[0]), frames_.size());
      frames_.resize(num_frames);
    }

    std::string to_str(std::size_t offset, const char * sep = "\n") const
    {
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
    }

    std::vector<void *> frames_;
  };

  //----------------------------------------------------------------
  // StackTrace::Private
  //
  struct StackTrace::Private : StackTraceLinux
  {};
#endif

  //----------------------------------------------------------------
  // StackTrace::StackTrace
  //
  StackTrace::StackTrace()
  {
    private_ = new StackTrace::Private();
  }

  //----------------------------------------------------------------
  // StackTrace::StackTrace
  //
  StackTrace::StackTrace(const StackTrace & bt):
    private_(new StackTrace::Private(*bt.private_))
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
      private_ = new StackTrace::Private(*bt.private_);
    }

    return *this;
  }

  //----------------------------------------------------------------
  // StackTrace::capture
  //
  void
  StackTrace::capture()
  {
    private_->capture();
  }

  //----------------------------------------------------------------
  // StackTrace::to_str
  //
  std::string
  StackTrace::to_str(std::size_t offset, const char * sep) const
  {
    return private_->to_str(offset, sep);
  }

  //----------------------------------------------------------------
  // get_stacktrace_str
  //
  std::string
  get_stacktrace_str(std::size_t offset, const char * sep)
  {
    StackTrace bt;
    bt.capture();
    return bt.to_str(offset, sep);
  }

}
