FLAG_IGCASE = true;

boolean flag_add_print = false;
boolean flag_fail_only = false;
boolean flag_replmode = false;
String exit_command = "exit";
String res_prefix = "";
int succ_count = 0;
int fail_count = 0;

void do_test(String path, String src0, String src, String res) {
	if(flag_add_print) {
		src = "(print " + src + ")";
	}
	if(FLAG_IGCASE) {
		res = res.convert(to:lower);
	}

	String r;
	if(flag_replmode) {
		// repl
		r = exec("echo \'" + src0 + src + "\n" + exit_command + "\' | " + path);
	} else {
		// file exec
		exec("echo \'" + src0 + "\' > a");
		exec("echo \'" + src + "\' >> a");
		r = exec(path + " a");
	}
	res = res_prefix + res;

	boolean succ = false;
	foreach(s in r.split("\n")) {
		if(FLAG_IGCASE) {
			s = s.convert(to:lower);
		}
		s = s.trim();
		if(s == res) {
			succ = true;
			break;
		}
	}
	src0 = src0.replace("\n", " ");
	if(succ) {
		succ_count++;
		if(!flag_fail_only) {
			if(src0 != "") OUT.println(src0);
			OUT.println(src + " ... OK");
		}
	} else {
		fail_count++;
		if(src0 != "") OUT.println(src0);
		OUT.println(src + " ... FAIL");
		OUT.println("\trequire: " + res);
		OUT.println("\tresult : " + r);
	}
}

void main(String[] args) {
	String lisppath = null;
	foreach(arg in args) {
		if(arg == "-p") {
			flag_add_print = true;
		} else if(arg == "-r") {
			flag_replmode = true;
		} else if(arg == "-fo") {
			flag_fail_only = true;
		} else if(arg.startsWith("-exit=")) {
			exit_command = arg.substring(6, arg.size-6);
		} else if(arg.startsWith("-rp=")) {
			res_prefix = arg.substring(4, arg.size-4);
		} else if(arg.startsWith("-")) {
			print "bad option: " + arg;
			return;
		} else {
			lisppath = arg;
		}
	}
	if(lisppath == null) {
		OUT.println("USAGE: [-p] [lisp path]");
		OUT.println("\t-p : add print expression to testcode, (print (test code))");
		OUT.println("\t-fo: print fail case only");
		OUT.println("\t-r : test in repl mode");
		OUT.println("\t-exit=xxx: exit command");
		OUT.println("\t-rp=xxx: result prefix");
		return;
	}
	String src0 = "";
	String src = "";
	String res = "";
	foreach(l in new InputStream("chkcode")) {
		l = l.trim();
		if(l.startsWith("#") || l.size == 0) continue;
		if(l.startsWith(">>>")) {
			src0 = src0 + l.substring(3, l.size - 3) + "\n";
		} else if(l.startsWith(">>")) {
			src = src + l.substring(2, l.size - 2);
		} else {
			res = l;
			do_test(lisppath, src0, src, res);
			src0 = "";
			src = "";
			res = "";
		}
	}
	OUT.println("--------------------------");
	OUT.println("result of " + lisppath);
	OUT.println("success: " + succ_count);
	OUT.println("fail   : " + fail_count);
	int total = succ_count + fail_count;
	OUT.println("total  : " + succ_count + " / " + total + " (" + 
		(succ_count * 100 / total) + "%)");
}

