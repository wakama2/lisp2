#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/PassManager.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
//#include <llvm/Analysis/Verifier.h>
//#include <llvm/Analysis/Pass.h>
//#include <llvm/Transforms/Scalar.h>
//#include <llvm/Target/TargetData.h>
#include "lisp.h"

using namespace llvm;

void llvm_init(Context *ctx) {
	LLVMContext &lctx = getGlobalContext();
	Module *m = new Module("lisp-func", lctx);

	ExecutionEngine *engine = EngineBuilder(m).setEngineKind(EngineKind::JIT).create();

	//FunctionPassManager fpm(m);
	//fpm.add(new TargetData(*engine->getTargetData()));
	//fpm.add(createBasicAliasAnalysisPass());
	//fpm.add(createInstructionCombiningPass());
	//fpm.add(createReassociatePass());
	//fpm.add(createCFGSimplificationPass());
	//fpm.doInitialization();

}

void llvm_compile(Context *ctx, Func *func) {

}

