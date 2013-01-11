//===-- cc1as_main.cpp - Clang Assembler  ---------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is the entry point to the clang -cc1as functionality, which implements
// the direct interface to the LLVM MC based assembler.
//
//===----------------------------------------------------------------------===//

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Driver/Arg.h"
#include "clang/Driver/ArgList.h"
#include "clang/Driver/CC1AsOptions.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "clang/Driver/OptTable.h"
#include "clang/Driver/Options.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCTargetAsmParser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/system_error.h"
using namespace clang;
using namespace clang::driver;
using namespace llvm;

namespace {

/// \brief Helper class for representing a single invocation of the assembler.
struct AssemblerInvocation {
  /// @name Target Options
  /// @{

  /// The name of the target triple to assemble for.
  std::string Triple;

  /// If given, the name of the target CPU to determine which instructions
  /// are legal.
  std::string CPU;

  /// The list of target specific features to enable or disable -- this should
  /// be a list of strings starting with '+' or '-'.
  std::vector<std::string> Features;

  /// @}
  /// @name Language Options
  /// @{

  std::vector<std::string> IncludePaths;
  unsigned NoInitialTextSection : 1;
  unsigned SaveTemporaryLabels : 1;
  unsigned GenDwarfForAssembly : 1;
  std::string DwarfDebugFlags;
  std::string DebugCompilationDir;
  std::string MainFileName;

  /// @}
  /// @name Frontend Options
  /// @{

  std::string InputFile;
  std::vector<std::string> LLVMArgs;
  std::string OutputPath;
  enum FileType {
    FT_Asm,  ///< Assembly (.s) output, transliterate mode.
    FT_Null, ///< No output, for timing purposes.
    FT_Obj   ///< Object file output.
  };
  FileType OutputType;
  unsigned ShowHelp : 1;
  unsigned ShowVersion : 1;

  /// @}
  /// @name Transliterate Options
  /// @{

  unsigned OutputAsmVariant;
  unsigned ShowEncoding : 1;
  unsigned ShowInst : 1;

  /// @}
  /// @name Assembler Options
  /// @{

  unsigned RelaxAll : 1;
  unsigned NoExecStack : 1;

  /// @}

public:
  AssemblerInvocation() {
    Triple = "";
    NoInitialTextSection = 0;
    InputFile = "-";
    OutputPath = "-";
    OutputType = FT_Asm;
    OutputAsmVariant = 0;
    ShowInst = 0;
    ShowEncoding = 0;
    RelaxAll = 0;
    NoExecStack = 0;
  }

  static bool CreateFromArgs(AssemblerInvocation &Res, const char **ArgBegin,
                             const char **ArgEnd, DiagnosticsEngine &Diags);
};

}

bool AssemblerInvocation::CreateFromArgs(AssemblerInvocation &Opts,
                                         const char **ArgBegin,
                                         const char **ArgEnd,
                                         DiagnosticsEngine &Diags) {
  using namespace clang::driver::cc1asoptions;
  bool Success = true;

  // Parse the arguments.
  OwningPtr<OptTable> OptTbl(createCC1AsOptTable());
  unsigned MissingArgIndex, MissingArgCount;
  OwningPtr<InputArgList> Args(
    OptTbl->ParseArgs(ArgBegin, ArgEnd,MissingArgIndex, MissingArgCount));

  // Check for missing argument error.
  if (MissingArgCount) {
    Diags.Report(diag::err_drv_missing_argument)
      << Args->getArgString(MissingArgIndex) << MissingArgCount;
    Success = false;
  }

  // Issue errors on unknown arguments.
  for (arg_iterator it = Args->filtered_begin(cc1asoptions::OPT_UNKNOWN),
         ie = Args->filtered_end(); it != ie; ++it) {
    Diags.Report(diag::err_drv_unknown_argument) << (*it) ->getAsString(*Args);
    Success = false;
  }

  // Construct the invocation.

  // Target Options
  Opts.Triple = llvm::Triple::normalize(Args->getLastArgValue(OPT_triple));
  Opts.CPU = Args->getLastArgValue(OPT_target_cpu);
  Opts.Features = Args->getAllArgValues(OPT_target_feature);

  // Use the default target triple if unspecified.
  if (Opts.Triple.empty())
    Opts.Triple = llvm::sys::getDefaultTargetTriple();

  // Language Options
  Opts.IncludePaths = Args->getAllArgValues(OPT_I);
  Opts.NoInitialTextSection = Args->hasArg(OPT_n);
  Opts.SaveTemporaryLabels = Args->hasArg(OPT_L);
  Opts.GenDwarfForAssembly = Args->hasArg(OPT_g);
  Opts.DwarfDebugFlags = Args->getLastArgValue(OPT_dwarf_debug_flags);
  Opts.DebugCompilationDir = Args->getLastArgValue(OPT_fdebug_compilation_dir);
  Opts.MainFileName = Args->getLastArgValue(OPT_main_file_name);

  // Frontend Options
  if (Args->hasArg(OPT_INPUT)) {
    bool First = true;
    for (arg_iterator it = Args->filtered_begin(OPT_INPUT),
           ie = Args->filtered_end(); it != ie; ++it, First=false) {
      const Arg *A = it;
      if (First)
        Opts.InputFile = A->getValue();
      else {
        Diags.Report(diag::err_drv_unknown_argument) << A->getAsString(*Args);
        Success = false;
      }
    }
  }
  Opts.LLVMArgs = Args->getAllArgValues(OPT_mllvm);
  if (Args->hasArg(OPT_fatal_warnings))
    Opts.LLVMArgs.push_back("-fatal-assembler-warnings");
  Opts.OutputPath = Args->getLastArgValue(OPT_o);
  if (Arg *A = Args->getLastArg(OPT_filetype)) {
    StringRef Name = A->getValue();
    unsigned OutputType = StringSwitch<unsigned>(Name)
      .Case("asm", FT_Asm)
      .Case("null", FT_Null)
      .Case("obj", FT_Obj)
      .Default(~0U);
    if (OutputType == ~0U) {
      Diags.Report(diag::err_drv_invalid_value)
        << A->getAsString(*Args) << Name;
      Success = false;
    } else
      Opts.OutputType = FileType(OutputType);
  }
  Opts.ShowHelp = Args->hasArg(OPT_help);
  Opts.ShowVersion = Args->hasArg(OPT_version);

  // Transliterate Options
  Opts.OutputAsmVariant = Args->getLastArgIntValue(OPT_output_asm_variant,
                                                   0, Diags);
  Opts.ShowEncoding = Args->hasArg(OPT_show_encoding);
  Opts.ShowInst = Args->hasArg(OPT_show_inst);

  // Assemble Options
  Opts.RelaxAll = Args->hasArg(OPT_relax_all);
  Opts.NoExecStack =  Args->hasArg(OPT_no_exec_stack);

  return Success;
}

static formatted_raw_ostream *GetOutputStream(AssemblerInvocation &Opts,
                                              DiagnosticsEngine &Diags,
                                              bool Binary) {
  if (Opts.OutputPath.empty())
    Opts.OutputPath = "-";

  // Make sure that the Out file gets unlinked from the disk if we get a
  // SIGINT.
  if (Opts.OutputPath != "-")
    sys::RemoveFileOnSignal(sys::Path(Opts.OutputPath));

  std::string Error;
  raw_fd_ostream *Out =
    new raw_fd_ostream(Opts.OutputPath.c_str(), Error,
                       (Binary ? raw_fd_ostream::F_Binary : 0));
  if (!Error.empty()) {
    Diags.Report(diag::err_fe_unable_to_open_output)
      << Opts.OutputPath << Error;
    return 0;
  }

  return new formatted_raw_ostream(*Out, formatted_raw_ostream::DELETE_STREAM);
}

static bool ExecuteAssembler(AssemblerInvocation &Opts,
                             DiagnosticsEngine &Diags) {
  // Get the target specific parser.
  std::string Error;
  const Target *TheTarget(TargetRegistry::lookupTarget(Opts.Triple, Error));
  if (!TheTarget) {
    Diags.Report(diag::err_target_unknown_triple) << Opts.Triple;
    return false;
  }

  OwningPtr<MemoryBuffer> BufferPtr;
  if (error_code ec = MemoryBuffer::getFileOrSTDIN(Opts.InputFile, BufferPtr)) {
    Error = ec.message();
    Diags.Report(diag::err_fe_error_reading) << Opts.InputFile;
    return false;
  }
  MemoryBuffer *Buffer = BufferPtr.take();

  SourceMgr SrcMgr;

  // Tell SrcMgr about this buffer, which is what the parser will pick up.
  SrcMgr.AddNewSourceBuffer(Buffer, SMLoc());

  // Record the location of the include directories so that the lexer can find
  // it later.
  SrcMgr.setIncludeDirs(Opts.IncludePaths);

  OwningPtr<MCAsmInfo> MAI(TheTarget->createMCAsmInfo(Opts.Triple));
  assert(MAI && "Unable to create target asm info!");

  OwningPtr<MCRegisterInfo> MRI(TheTarget->createMCRegInfo(Opts.Triple));
  assert(MRI && "Unable to create target register info!");

  bool IsBinary = Opts.OutputType == AssemblerInvocation::FT_Obj;
  formatted_raw_ostream *Out = GetOutputStream(Opts, Diags, IsBinary);
  if (!Out)
    return false;

  // FIXME: This is not pretty. MCContext has a ptr to MCObjectFileInfo and
  // MCObjectFileInfo needs a MCContext reference in order to initialize itself.
  OwningPtr<MCObjectFileInfo> MOFI(new MCObjectFileInfo());
  MCContext Ctx(*MAI, *MRI, MOFI.get(), &SrcMgr);
  // FIXME: Assembler behavior can change with -static.
  MOFI->InitMCObjectFileInfo(Opts.Triple,
                             Reloc::Default, CodeModel::Default, Ctx);
  if (Opts.SaveTemporaryLabels)
    Ctx.setAllowTemporaryLabels(false);
  if (Opts.GenDwarfForAssembly)
    Ctx.setGenDwarfForAssembly(true);
  if (!Opts.DwarfDebugFlags.empty())
    Ctx.setDwarfDebugFlags(StringRef(Opts.DwarfDebugFlags));
  if (!Opts.DebugCompilationDir.empty())
    Ctx.setCompilationDir(Opts.DebugCompilationDir);
  if (!Opts.MainFileName.empty())
    Ctx.setMainFileName(StringRef(Opts.MainFileName));

  // Build up the feature string from the target feature list.
  std::string FS;
  if (!Opts.Features.empty()) {
    FS = Opts.Features[0];
    for (unsigned i = 1, e = Opts.Features.size(); i != e; ++i)
      FS += "," + Opts.Features[i];
  }

  OwningPtr<MCStreamer> Str;

  OwningPtr<MCInstrInfo> MCII(TheTarget->createMCInstrInfo());
  OwningPtr<MCSubtargetInfo>
    STI(TheTarget->createMCSubtargetInfo(Opts.Triple, Opts.CPU, FS));

  // FIXME: There is a bit of code duplication with addPassesToEmitFile.
  if (Opts.OutputType == AssemblerInvocation::FT_Asm) {
    MCInstPrinter *IP =
      TheTarget->createMCInstPrinter(Opts.OutputAsmVariant, *MAI, *MCII, *MRI,
                                     *STI);
    MCCodeEmitter *CE = 0;
    MCAsmBackend *MAB = 0;
    if (Opts.ShowEncoding) {
      CE = TheTarget->createMCCodeEmitter(*MCII, *MRI, *STI, Ctx);
      MAB = TheTarget->createMCAsmBackend(Opts.Triple, Opts.CPU);
    }
    Str.reset(TheTarget->createAsmStreamer(Ctx, *Out, /*asmverbose*/true,
                                           /*useLoc*/ true,
                                           /*useCFI*/ true,
                                           /*useDwarfDirectory*/ true,
                                           IP, CE, MAB,
                                           Opts.ShowInst));
  } else if (Opts.OutputType == AssemblerInvocation::FT_Null) {
    Str.reset(createNullStreamer(Ctx));
  } else {
    assert(Opts.OutputType == AssemblerInvocation::FT_Obj &&
           "Invalid file type!");
    MCCodeEmitter *CE = TheTarget->createMCCodeEmitter(*MCII, *MRI, *STI, Ctx);
    MCAsmBackend *MAB = TheTarget->createMCAsmBackend(Opts.Triple, Opts.CPU);
    Str.reset(TheTarget->createMCObjectStreamer(Opts.Triple, Ctx, *MAB, *Out,
                                                CE, Opts.RelaxAll,
                                                Opts.NoExecStack));
    Str.get()->InitSections();
  }

  OwningPtr<MCAsmParser> Parser(createMCAsmParser(SrcMgr, Ctx,
                                                  *Str.get(), *MAI));
  OwningPtr<MCTargetAsmParser> TAP(TheTarget->createMCAsmParser(*STI, *Parser));
  if (!TAP) {
    Diags.Report(diag::err_target_unknown_triple) << Opts.Triple;
    return false;
  }

  Parser->setTargetParser(*TAP.get());

  bool Success = !Parser->Run(Opts.NoInitialTextSection);

  // Close the output.
  delete Out;

  // Delete output on errors.
  if (!Success && Opts.OutputPath != "-")
    sys::Path(Opts.OutputPath).eraseFromDisk();

  return Success;
}

static void LLVMErrorHandler(void *UserData, const std::string &Message) {
  DiagnosticsEngine &Diags = *static_cast<DiagnosticsEngine*>(UserData);

  Diags.Report(diag::err_fe_error_backend) << Message;

  // We cannot recover from llvm errors.
  exit(1);
}

int cc1as_main(const char **ArgBegin, const char **ArgEnd,
               const char *Argv0, void *MainAddr) {
  // Print a stack trace if we signal out.
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(ArgEnd - ArgBegin, ArgBegin);
  llvm_shutdown_obj Y;  // Call llvm_shutdown() on exit.

  // Initialize targets and assembly printers/parsers.
  InitializeAllTargetInfos();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();

  // Construct our diagnostic client.
  IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions();
  TextDiagnosticPrinter *DiagClient
    = new TextDiagnosticPrinter(errs(), &*DiagOpts);
  DiagClient->setPrefix("clang -cc1as");
  IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());
  DiagnosticsEngine Diags(DiagID, &*DiagOpts, DiagClient);

  // Set an error handler, so that any LLVM backend diagnostics go through our
  // error handler.
  ScopedFatalErrorHandler FatalErrorHandler
    (LLVMErrorHandler, static_cast<void*>(&Diags));

  // Parse the arguments.
  AssemblerInvocation Asm;
  if (!AssemblerInvocation::CreateFromArgs(Asm, ArgBegin, ArgEnd, Diags))
    return 1;

  // Honor -help.
  if (Asm.ShowHelp) {
    OwningPtr<driver::OptTable> Opts(driver::createCC1AsOptTable());
    Opts->PrintHelp(llvm::outs(), "clang -cc1as", "Clang Integrated Assembler");
    return 0;
  }

  // Honor -version.
  //
  // FIXME: Use a better -version message?
  if (Asm.ShowVersion) {
    llvm::cl::PrintVersionMessage();
    return 0;
  }

  // Honor -mllvm.
  //
  // FIXME: Remove this, one day.
  if (!Asm.LLVMArgs.empty()) {
    unsigned NumArgs = Asm.LLVMArgs.size();
    const char **Args = new const char*[NumArgs + 2];
    Args[0] = "clang (LLVM option parsing)";
    for (unsigned i = 0; i != NumArgs; ++i)
      Args[i + 1] = Asm.LLVMArgs[i].c_str();
    Args[NumArgs + 1] = 0;
    llvm::cl::ParseCommandLineOptions(NumArgs + 1, Args);
  }

  // Execute the invocation, unless there were parsing errors.
  bool Success = false;
  if (!Diags.hasErrorOccurred())
    Success = ExecuteAssembler(Asm, Diags);

  // If any timers were active but haven't been destroyed yet, print their
  // results now.
  TimerGroup::printAll(errs());

  return !Success;
}

//===-- cc1_main.cpp - Clang CC1 Compiler Frontend ------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is the entry point to the clang -cc1 functionality, which implements the
// core compiler functionality along with a number of additional tools for
// demonstration and testing purposes.
//
//===----------------------------------------------------------------------===//

#include "clang/Driver/Arg.h"
#include "clang/Driver/ArgList.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "clang/Driver/OptTable.h"
#include "clang/Driver/Options.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/TextDiagnosticBuffer.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/FrontendTool/Utils.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/LinkAllPasses.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdio>
using namespace clang;

//===----------------------------------------------------------------------===//
// Main driver
//===----------------------------------------------------------------------===//

#if 0
static void LLVMErrorHandler(void *UserData, const std::string &Message) {
  DiagnosticsEngine &Diags = *static_cast<DiagnosticsEngine*>(UserData);

  Diags.Report(diag::err_fe_error_backend) << Message;

  // Run the interrupt handlers to make sure any special cleanups get done, in
  // particular that we remove files registered with RemoveFileOnSignal.
  llvm::sys::RunInterruptHandlers();

  // We cannot recover from llvm errors.  When reporting a fatal error, exit
  // with status 70.  For BSD systems this is defined as an internal software
  // error.  This notifies the driver to report diagnostics information.
  exit(70);
}
#endif

int cc1_main(const char **ArgBegin, const char **ArgEnd,
             const char *Argv0, void *MainAddr) {
  OwningPtr<CompilerInstance> Clang(new CompilerInstance());
  IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());

  // Initialize targets first, so that --version shows registered targets.
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

  // Buffer diagnostics from argument parsing so that we can output them using a
  // well formed diagnostic object.
  IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions();
  TextDiagnosticBuffer *DiagsBuffer = new TextDiagnosticBuffer;
  DiagnosticsEngine Diags(DiagID, &*DiagOpts, DiagsBuffer);
  bool Success;
  Success = CompilerInvocation::CreateFromArgs(Clang->getInvocation(),
                                               ArgBegin, ArgEnd, Diags);

  // Infer the builtin include path if unspecified.
  if (Clang->getHeaderSearchOpts().UseBuiltinIncludes &&
      Clang->getHeaderSearchOpts().ResourceDir.empty())
    Clang->getHeaderSearchOpts().ResourceDir =
      CompilerInvocation::GetResourcesPath(Argv0, MainAddr);

  // Create the actual diagnostics engine.
  Clang->createDiagnostics(ArgEnd - ArgBegin, const_cast<char**>(ArgBegin));
  if (!Clang->hasDiagnostics())
    return 1;

  // Set an error handler, so that any LLVM backend diagnostics go through our
  // error handler.
  llvm::install_fatal_error_handler(LLVMErrorHandler,
                                  static_cast<void*>(&Clang->getDiagnostics()));

  DiagsBuffer->FlushDiagnostics(Clang->getDiagnostics());
  if (!Success)
    return 1;

  // Execute the frontend actions.
  Success = ExecuteCompilerInvocation(Clang.get());

  // If any timers were active but haven't been destroyed yet, print their
  // results now.  This happens in -disable-free mode.
  llvm::TimerGroup::printAll(llvm::errs());

  // Our error handler depends on the Diagnostics object, which we're
  // potentially about to delete. Uninstall the handler now so that any
  // later errors use the default handling behavior instead.
  llvm::remove_fatal_error_handler();

  // When running with -disable-free, don't do any destruction or shutdown.
  if (Clang->getFrontendOpts().DisableFree) {
    if (llvm::AreStatisticsEnabled() || Clang->getFrontendOpts().ShowStats)
      llvm::PrintStatistics();
    Clang.take();
    return !Success;
  }

  // Managed static deconstruction. Useful for making things like
  // -time-passes usable.
  llvm::llvm_shutdown();

  return !Success;
}



//===-- driver.cpp - Clang GCC-Compatible Driver --------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This is the entry point to the clang driver; it is a thin wrapper
// for functionality in the Driver clang library.
//
//===----------------------------------------------------------------------===//

#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Driver/ArgList.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "clang/Driver/OptTable.h"
#include "clang/Driver/Option.h"
#include "clang/Driver/Options.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Frontend/Utils.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/system_error.h"
#include <cctype>
using namespace clang;
using namespace clang::driver;

llvm::sys::Path GetExecutablePath(const char *Argv0, bool CanonicalPrefixes) {
  if (!CanonicalPrefixes)
    return llvm::sys::Path(Argv0);

  // This just needs to be some symbol in the binary; C++ doesn't
  // allow taking the address of ::main however.
  void *P = (void*) (intptr_t) GetExecutablePath;
  return llvm::sys::Path::GetMainExecutable(Argv0, P);
}

static const char *SaveStringInSet(std::set<std::string> &SavedStrings,
                                   StringRef S) {
  return SavedStrings.insert(S).first->c_str();
}

/// ApplyQAOverride - Apply a list of edits to the input argument lists.
///
/// The input string is a space separate list of edits to perform,
/// they are applied in order to the input argument lists. Edits
/// should be one of the following forms:
///
///  '#': Silence information about the changes to the command line arguments.
///
///  '^': Add FOO as a new argument at the beginning of the command line.
///
///  '+': Add FOO as a new argument at the end of the command line.
///
///  's/XXX/YYY/': Substitute the regular expression XXX with YYY in the command
///  line.
///
///  'xOPTION': Removes all instances of the literal argument OPTION.
///
///  'XOPTION': Removes all instances of the literal argument OPTION,
///  and the following argument.
///
///  'Ox': Removes all flags matching 'O' or 'O[sz0-9]' and adds 'Ox'
///  at the end of the command line.
///
/// \param OS - The stream to write edit information to.
/// \param Args - The vector of command line arguments.
/// \param Edit - The override command to perform.
/// \param SavedStrings - Set to use for storing string representations.
static void ApplyOneQAOverride(raw_ostream &OS,
                               SmallVectorImpl<const char*> &Args,
                               StringRef Edit,
                               std::set<std::string> &SavedStrings) {
  // This does not need to be efficient.

  if (Edit[0] == '^') {
    const char *Str =
      SaveStringInSet(SavedStrings, Edit.substr(1));
    OS << "### Adding argument " << Str << " at beginning\n";
    Args.insert(Args.begin() + 1, Str);
  } else if (Edit[0] == '+') {
    const char *Str =
      SaveStringInSet(SavedStrings, Edit.substr(1));
    OS << "### Adding argument " << Str << " at end\n";
    Args.push_back(Str);
  } else if (Edit[0] == 's' && Edit[1] == '/' && Edit.endswith("/") &&
             Edit.slice(2, Edit.size()-1).find('/') != StringRef::npos) {
    StringRef MatchPattern = Edit.substr(2).split('/').first;
    StringRef ReplPattern = Edit.substr(2).split('/').second;
    ReplPattern = ReplPattern.slice(0, ReplPattern.size()-1);

    for (unsigned i = 1, e = Args.size(); i != e; ++i) {
      std::string Repl = llvm::Regex(MatchPattern).sub(ReplPattern, Args[i]);

      if (Repl != Args[i]) {
        OS << "### Replacing '" << Args[i] << "' with '" << Repl << "'\n";
        Args[i] = SaveStringInSet(SavedStrings, Repl);
      }
    }
  } else if (Edit[0] == 'x' || Edit[0] == 'X') {
    std::string Option = Edit.substr(1, std::string::npos);
    for (unsigned i = 1; i < Args.size();) {
      if (Option == Args[i]) {
        OS << "### Deleting argument " << Args[i] << '\n';
        Args.erase(Args.begin() + i);
        if (Edit[0] == 'X') {
          if (i < Args.size()) {
            OS << "### Deleting argument " << Args[i] << '\n';
            Args.erase(Args.begin() + i);
          } else
            OS << "### Invalid X edit, end of command line!\n";
        }
      } else
        ++i;
    }
  } else if (Edit[0] == 'O') {
    for (unsigned i = 1; i < Args.size();) {
      const char *A = Args[i];
      if (A[0] == '-' && A[1] == 'O' &&
          (A[2] == '\0' ||
           (A[3] == '\0' && (A[2] == 's' || A[2] == 'z' ||
                             ('0' <= A[2] && A[2] <= '9'))))) {
        OS << "### Deleting argument " << Args[i] << '\n';
        Args.erase(Args.begin() + i);
      } else
        ++i;
    }
    OS << "### Adding argument " << Edit << " at end\n";
    Args.push_back(SaveStringInSet(SavedStrings, '-' + Edit.str()));
  } else {
    OS << "### Unrecognized edit: " << Edit << "\n";
  }
}

/// ApplyQAOverride - Apply a comma separate list of edits to the
/// input argument lists. See ApplyOneQAOverride.
static void ApplyQAOverride(SmallVectorImpl<const char*> &Args,
                            const char *OverrideStr,
                            std::set<std::string> &SavedStrings) {
  raw_ostream *OS = &llvm::errs();

  if (OverrideStr[0] == '#') {
    ++OverrideStr;
    OS = &llvm::nulls();
  }

  *OS << "### QA_OVERRIDE_GCC3_OPTIONS: " << OverrideStr << "\n";

  // This does not need to be efficient.

  const char *S = OverrideStr;
  while (*S) {
    const char *End = ::strchr(S, ' ');
    if (!End)
      End = S + strlen(S);
    if (End != S)
      ApplyOneQAOverride(*OS, Args, std::string(S, End), SavedStrings);
    S = End;
    if (*S != '\0')
      ++S;
  }
}

extern int cc1_main(const char **ArgBegin, const char **ArgEnd,
                    const char *Argv0, void *MainAddr);
extern int cc1as_main(const char **ArgBegin, const char **ArgEnd,
                      const char *Argv0, void *MainAddr);

static void ExpandArgsFromBuf(const char *Arg,
                              SmallVectorImpl<const char*> &ArgVector,
                              std::set<std::string> &SavedStrings) {
  const char *FName = Arg + 1;
  OwningPtr<llvm::MemoryBuffer> MemBuf;
  if (llvm::MemoryBuffer::getFile(FName, MemBuf)) {
    ArgVector.push_back(SaveStringInSet(SavedStrings, Arg));
    return;
  }

  const char *Buf = MemBuf->getBufferStart();
  char InQuote = ' ';
  std::string CurArg;

  for (const char *P = Buf; ; ++P) {
    if (*P == '\0' || (isspace(*P) && InQuote == ' ')) {
      if (!CurArg.empty()) {

        if (CurArg[0] != '@') {
          ArgVector.push_back(SaveStringInSet(SavedStrings, CurArg));
        } else {
          ExpandArgsFromBuf(CurArg.c_str(), ArgVector, SavedStrings);
        }

        CurArg = "";
      }
      if (*P == '\0')
        break;
      else
        continue;
    }

    if (isspace(*P)) {
      if (InQuote != ' ')
        CurArg.push_back(*P);
      continue;
    }

    if (*P == '"' || *P == '\'') {
      if (InQuote == *P)
        InQuote = ' ';
      else if (InQuote == ' ')
        InQuote = *P;
      else
        CurArg.push_back(*P);
      continue;
    }

    if (*P == '\\') {
      ++P;
      if (*P != '\0')
        CurArg.push_back(*P);
      continue;
    }
    CurArg.push_back(*P);
  }
}

static void ExpandArgv(int argc, const char **argv,
                       SmallVectorImpl<const char*> &ArgVector,
                       std::set<std::string> &SavedStrings) {
  for (int i = 0; i < argc; ++i) {
    const char *Arg = argv[i];
    if (Arg[0] != '@') {
      ArgVector.push_back(SaveStringInSet(SavedStrings, std::string(Arg)));
      continue;
    }

    ExpandArgsFromBuf(Arg, ArgVector, SavedStrings);
  }
}

static void ParseProgName(SmallVectorImpl<const char *> &ArgVector,
                          std::set<std::string> &SavedStrings,
                          Driver &TheDriver)
{
  // Try to infer frontend type and default target from the program name.

  // suffixes[] contains the list of known driver suffixes.
  // Suffixes are compared against the program name in order.
  // If there is a match, the frontend type is updated as necessary (CPP/C++).
  // If there is no match, a second round is done after stripping the last
  // hyphen and everything following it. This allows using something like
  // "clang++-2.9".

  // If there is a match in either the first or second round,
  // the function tries to identify a target as prefix. E.g.
  // "x86_64-linux-clang" as interpreted as suffix "clang" with
  // target prefix "x86_64-linux". If such a target prefix is found,
  // is gets added via -target as implicit first argument.
  static const struct {
    const char *Suffix;
    bool IsCXX;
    bool IsCPP;
  } suffixes [] = {
    { "clang", false, false },
    { "clang++", true, false },
    { "clang-c++", true, false },
    { "clang-cc", false, false },
    { "clang-cpp", false, true },
    { "clang-g++", true, false },
    { "clang-gcc", false, false },
    { "cc", false, false },
    { "cpp", false, true },
    { "++", true, false },
  };
  std::string ProgName(llvm::sys::path::stem(ArgVector[0]));
  StringRef ProgNameRef(ProgName);
  StringRef Prefix;

  for (int Components = 2; Components; --Components) {
    bool FoundMatch = false;
    size_t i;

    for (i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); ++i) {
      if (ProgNameRef.endswith(suffixes[i].Suffix)) {
        FoundMatch = true;
        if (suffixes[i].IsCXX)
          TheDriver.CCCIsCXX = true;
        if (suffixes[i].IsCPP)
          TheDriver.CCCIsCPP = true;
        break;
      }
    }

    if (FoundMatch) {
      StringRef::size_type LastComponent = ProgNameRef.rfind('-',
        ProgNameRef.size() - strlen(suffixes[i].Suffix));
      if (LastComponent != StringRef::npos)
        Prefix = ProgNameRef.slice(0, LastComponent);
      break;
    }

    StringRef::size_type LastComponent = ProgNameRef.rfind('-');
    if (LastComponent == StringRef::npos)
      break;
    ProgNameRef = ProgNameRef.slice(0, LastComponent);
  }

  if (Prefix.empty())
    return;

  std::string IgnoredError;
  if (llvm::TargetRegistry::lookupTarget(Prefix, IgnoredError)) {
    SmallVectorImpl<const char *>::iterator it = ArgVector.begin();
    if (it != ArgVector.end())
      ++it;
    ArgVector.insert(it, SaveStringInSet(SavedStrings, Prefix));
    ArgVector.insert(it,
      SaveStringInSet(SavedStrings, std::string("-target")));
  }
}

int main(int argc_, const char **argv_) {
  llvm::sys::PrintStackTraceOnErrorSignal();
  llvm::PrettyStackTraceProgram X(argc_, argv_);

  std::set<std::string> SavedStrings;
  SmallVector<const char*, 256> argv;

  ExpandArgv(argc_, argv_, argv, SavedStrings);

  // Handle -cc1 integrated tools.
  if (argv.size() > 1 && StringRef(argv[1]).startswith("-cc1")) {
    StringRef Tool = argv[1] + 4;

    if (Tool == "")
      return cc1_main(argv.data()+2, argv.data()+argv.size(), argv[0],
                      (void*) (intptr_t) GetExecutablePath);
    if (Tool == "as")
      return cc1as_main(argv.data()+2, argv.data()+argv.size(), argv[0],
                      (void*) (intptr_t) GetExecutablePath);

    // Reject unknown tools.
    llvm::errs() << "error: unknown integrated tool '" << Tool << "'\n";
    return 1;
  }

  bool CanonicalPrefixes = true;
  for (int i = 1, size = argv.size(); i < size; ++i) {
    if (StringRef(argv[i]) == "-no-canonical-prefixes") {
      CanonicalPrefixes = false;
      break;
    }
  }

  llvm::sys::Path Path = GetExecutablePath(argv[0], CanonicalPrefixes);

  IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions;
  {
    // Note that ParseDiagnosticArgs() uses the cc1 option table.
    OwningPtr<OptTable> CC1Opts(createDriverOptTable());
    unsigned MissingArgIndex, MissingArgCount;
    OwningPtr<InputArgList> Args(CC1Opts->ParseArgs(argv.begin()+1, argv.end(),
                                            MissingArgIndex, MissingArgCount));
    // We ignore MissingArgCount and the return value of ParseDiagnosticArgs.
    // Any errors that would be diagnosed here will also be diagnosed later,
    // when the DiagnosticsEngine actually exists.
    (void) ParseDiagnosticArgs(*DiagOpts, *Args);
  }
  // Now we can create the DiagnosticsEngine with a properly-filled-out
  // DiagnosticOptions instance.
  TextDiagnosticPrinter *DiagClient
    = new TextDiagnosticPrinter(llvm::errs(), &*DiagOpts);
  DiagClient->setPrefix(llvm::sys::path::stem(Path.str()));
  IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());

  DiagnosticsEngine Diags(DiagID, &*DiagOpts, DiagClient);
  ProcessWarningOptions(Diags, *DiagOpts);

  Driver TheDriver(Path.str(), llvm::sys::getDefaultTargetTriple(),
                   "a.out", Diags);

  // Attempt to find the original path used to invoke the driver, to determine
  // the installed path. We do this manually, because we want to support that
  // path being a symlink.
  {
    SmallString<128> InstalledPath(argv[0]);

    // Do a PATH lookup, if there are no directory components.
    if (llvm::sys::path::filename(InstalledPath) == InstalledPath) {
      llvm::sys::Path Tmp = llvm::sys::Program::FindProgramByName(
        llvm::sys::path::filename(InstalledPath.str()));
      if (!Tmp.empty())
        InstalledPath = Tmp.str();
    }
    llvm::sys::fs::make_absolute(InstalledPath);
    InstalledPath = llvm::sys::path::parent_path(InstalledPath);
    bool exists;
    if (!llvm::sys::fs::exists(InstalledPath.str(), exists) && exists)
      TheDriver.setInstalledDir(InstalledPath);
  }

  llvm::InitializeAllTargets();
  ParseProgName(argv, SavedStrings, TheDriver);

  // Handle CC_PRINT_OPTIONS and CC_PRINT_OPTIONS_FILE.
  TheDriver.CCPrintOptions = !!::getenv("CC_PRINT_OPTIONS");
  if (TheDriver.CCPrintOptions)
    TheDriver.CCPrintOptionsFilename = ::getenv("CC_PRINT_OPTIONS_FILE");

  // Handle CC_PRINT_HEADERS and CC_PRINT_HEADERS_FILE.
  TheDriver.CCPrintHeaders = !!::getenv("CC_PRINT_HEADERS");
  if (TheDriver.CCPrintHeaders)
    TheDriver.CCPrintHeadersFilename = ::getenv("CC_PRINT_HEADERS_FILE");

  // Handle CC_LOG_DIAGNOSTICS and CC_LOG_DIAGNOSTICS_FILE.
  TheDriver.CCLogDiagnostics = !!::getenv("CC_LOG_DIAGNOSTICS");
  if (TheDriver.CCLogDiagnostics)
    TheDriver.CCLogDiagnosticsFilename = ::getenv("CC_LOG_DIAGNOSTICS_FILE");

  // Handle QA_OVERRIDE_GCC3_OPTIONS and CCC_ADD_ARGS, used for editing a
  // command line behind the scenes.
  if (const char *OverrideStr = ::getenv("QA_OVERRIDE_GCC3_OPTIONS")) {
    // FIXME: Driver shouldn't take extra initial argument.
    ApplyQAOverride(argv, OverrideStr, SavedStrings);
  } else if (const char *Cur = ::getenv("CCC_ADD_ARGS")) {
    // FIXME: Driver shouldn't take extra initial argument.
    std::vector<const char*> ExtraArgs;

    for (;;) {
      const char *Next = strchr(Cur, ',');

      if (Next) {
        ExtraArgs.push_back(SaveStringInSet(SavedStrings,
                                            std::string(Cur, Next)));
        Cur = Next + 1;
      } else {
        if (*Cur != '\0')
          ExtraArgs.push_back(SaveStringInSet(SavedStrings, Cur));
        break;
      }
    }

    argv.insert(&argv[1], ExtraArgs.begin(), ExtraArgs.end());
  }

  OwningPtr<Compilation> C(TheDriver.BuildCompilation(argv));
  int Res = 0;
  const Command *FailingCommand = 0;
  if (C.get())
    Res = TheDriver.ExecuteCompilation(*C, FailingCommand);

  // Force a crash to test the diagnostics.
  if (::getenv("FORCE_CLANG_DIAGNOSTICS_CRASH")) {
    Diags.Report(diag::err_drv_force_crash) << "FORCE_CLANG_DIAGNOSTICS_CRASH";
    Res = -1;
  }

  // If result status is < 0, then the driver command signalled an error.
  // If result status is 70, then the driver command reported a fatal error.
  // In these cases, generate additional diagnostic information if possible.
  if (Res < 0 || Res == 70)
    TheDriver.generateCompilationDiagnostics(*C, FailingCommand);

  // If any timers were active but haven't been destroyed yet, print their
  // results now.  This happens in -disable-free mode.
  llvm::TimerGroup::printAll(llvm::errs());
  
  llvm::llvm_shutdown();

#ifdef _WIN32
  // Exit status should not be negative on Win32, unless abnormal termination.
  // Once abnormal termiation was caught, negative status should not be
  // propagated.
  if (Res < 0)
    Res = 1;
#endif

  return Res;
}
