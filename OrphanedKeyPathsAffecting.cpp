//
// OrphanedKeyPathsAffecting.cpp
// Created by Jonathon Mah on 2013-05-14.
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

namespace {

class OrphanedKeyPathCheckingConsumer : public ASTConsumer {
public:
  OrphanedKeyPathCheckingConsumer(const CompilerInstance &compiler)
    : ASTConsumer()
    , compiler(compiler)
    , prefix("keyPathsForValuesAffecting")
  { }

  virtual bool HandleTopLevelDecl(DeclGroupRef DG) {
    using namespace std;

    for (DeclGroupRef::iterator i = DG.begin(), e = DG.end(); i != e; ++i) {
      if (const ObjCMethodDecl *MD = dyn_cast<ObjCMethodDecl>(*i)) {
        if (MD->getSelector().getNumArgs() > 0)
          continue;

        string name = MD->getNameAsString();
        if (MD->isClassMethod() && name.length() > prefix.length())
            if (equal(prefix.begin(), prefix.end(), name.begin()))
              affectingSelectorToLocation[MD] = MD->getSelectorStartLoc().getLocWithOffset(prefix.length());
      }
    }
    return true;
  }

  virtual void HandleTranslationUnit(ASTContext &Ctx) {
    using namespace std;

    Ctx.Selectors.getNullarySelector(&Ctx.Idents.get(""));
    for (map<const ObjCMethodDecl *, SourceLocation>::const_iterator i = affectingSelectorToLocation.begin(), e = affectingSelectorToLocation.end(); i != e; ++i) {
      string selName = i->first->getNameAsString();
      string capitalizedKeyName = selName.substr(prefix.length());
      const ObjCInterfaceDecl *CI = i->first->getClassInterface();

      if (classHasGetter(Ctx, CI, capitalizedKeyName))
        continue; // e.g. "keyPathsForValuesAffectingURL" can be satisfied by "URL".
      string lowercaseKeyName = capitalizedKeyName;
      lowercaseKeyName[0] = tolower(capitalizedKeyName[0]);
      if (classHasGetter(Ctx, CI, lowercaseKeyName))
        continue; // e.g. "keyPathsForValuesAffectingFoo" can be satisfied by "foo"
      if (classHasGetter(Ctx, CI, "is" + capitalizedKeyName))
        continue; // e.g. "keyPathsForValuesAffectingFoo" can be satisfied by "isFoo"
      if (classHasGetter(Ctx, CI, "countOf" + capitalizedKeyName))
        continue; // e.g. "keyPathsForValuesAffectingFoo" can be satisfied by "countOfFoo" (indexed accessor - uncommon, so check this last)

      FullSourceLoc fullSourceLocation(i->second, compiler.getSourceManager());
      DiagnosticsEngine &de = compiler.getDiagnostics();
      unsigned id = de.getCustomDiagID(DiagnosticsEngine::Warning, "Corresponding getter not found for key '" + lowercaseKeyName + "'");
      DiagnosticBuilder B = de.Report(fullSourceLocation, id);
    }
  }

private:
  const CompilerInstance &compiler;
  std::string prefix;
  std::map<const ObjCMethodDecl *, SourceLocation> affectingSelectorToLocation;

  bool classHasGetter(ASTContext &Ctx, const ObjCInterfaceDecl *CI, std::string GetterString) {
    Selector Sel = Ctx.Selectors.getNullarySelector(&Ctx.Idents.get(GetterString));
    // It _seems_: lookupInstanceMethod checks public interface of class and superclasses
    // lookupPrivateMethod checks methods of the target class defined in implementation, not declared in interface.
    return CI->lookupInstanceMethod(Sel) != 0 || CI->lookupPrivateMethod(Sel) != 0;
  }
};


class NullConsumer : public ASTConsumer {
public:
  NullConsumer()
    : ASTConsumer()
  { }
};


class CheckOrphanedKeyPathsAction : public PluginASTAction {
protected:
  ASTConsumer *CreateASTConsumer(CompilerInstance &compiler, llvm::StringRef) {
    LangOptions const opts = compiler.getLangOpts();
    if (opts.ObjC1 || opts.ObjC2)
      return new OrphanedKeyPathCheckingConsumer(compiler);
    else
      return new NullConsumer();
  }

  bool ParseArgs(const CompilerInstance &compiler,
                 const std::vector<std::string>& args) {
    return true;
  }
};

}

static FrontendPluginRegistry::Add<CheckOrphanedKeyPathsAction>
X("check-orphaned-key-paths-affecting", "check for declarations of +keyPathsForValuesAffecting<Foo> without a corresponding -<foo> declaration");
