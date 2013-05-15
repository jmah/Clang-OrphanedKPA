// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.

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
    , dependentKeyPathSelectorPrefix("keyPathsForValuesAffecting")
  { }

  virtual bool HandleTopLevelDecl(DeclGroupRef DG) {
    using namespace std;

    for (DeclGroupRef::iterator i = DG.begin(), e = DG.end(); i != e; ++i) {
      if (const ObjCMethodDecl *MD = dyn_cast<ObjCMethodDecl>(*i)) {
        if (MD->getSelector().getNumArgs() > 0)
          continue;

        string name = MD->getNameAsString();
        if (MD->isClassMethod()) {
          if (name.length() > dependentKeyPathSelectorPrefix.length())
            if (equal(dependentKeyPathSelectorPrefix.begin(),
                      dependentKeyPathSelectorPrefix.end(),
                      name.begin()))
              affectingSelectorToLocation[name] = MD->getSelectorStartLoc().getLocWithOffset(dependentKeyPathSelectorPrefix.length());

        } else { // instance method
          getterSet.insert(name);
        }
      }
    }
    return true;
  }

  virtual void HandleTranslationUnit(ASTContext &Ctx) {
    using namespace std;

    for (map<const string, SourceLocation>::const_iterator i = affectingSelectorToLocation.begin(), e = affectingSelectorToLocation.end(); i != e; ++i) {
      string capitalizedKeyName = i->first.substr(dependentKeyPathSelectorPrefix.length());
      if (getterSet.count(capitalizedKeyName) > 0)
        continue; // e.g. "keyPathsForValuesAffectingURL" can be satisfied by "URL".
      string lowercaseKeyName = capitalizedKeyName;
      lowercaseKeyName[0] = tolower(capitalizedKeyName[0]);
      if (getterSet.count(lowercaseKeyName) > 0)
        continue; // e.g. "keyPathsForValuesAffectingFoo" can be satisfied by "foo"
      if (getterSet.count("is" + capitalizedKeyName) > 0)
        continue; // e.g. "keyPathsForValuesAffectingFoo" can be satisfied by "isFoo"
      if (getterSet.count("countOf" + capitalizedKeyName) > 0)
        continue; // e.g. "keyPathsForValuesAffectingFoo" can be satisfied by "countOfFoo" (indexed accessor - uncommon, so check this last)

      FullSourceLoc fullSourceLocation(i->second, compiler.getSourceManager());
      DiagnosticsEngine &de = compiler.getDiagnostics();
      unsigned id = de.getCustomDiagID(DiagnosticsEngine::Warning, "Corresponding getter not found for key '" + lowercaseKeyName + "'");
      DiagnosticBuilder B = de.Report(fullSourceLocation, id);
    }
  }

private:
  const CompilerInstance &compiler;
  std::string dependentKeyPathSelectorPrefix;
  std::map<const std::string, SourceLocation> affectingSelectorToLocation;
  std::set<std::string> getterSet;
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
