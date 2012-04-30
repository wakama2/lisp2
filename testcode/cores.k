N = 10;

void main() {
	res = [];
	for(int i=2; i<8; i++) {
		float sum = 0.0;
		for(int j=0; j<N; j++) {
			String s = exec("time -p ./lisp fib.lisp -worker " + i + " >& a && cat a");
			tk = s.split("\n");
			if(tk[0] != "102334155") {
				print "ERROR RESULT\n";
				print s;
				return;
			}
			time = (float)tk[1].match($/real\s+(\d+.\d+)/)[1]
			sum += time;
			OUT.println(time);
		}
		float ave = sum / N;
		OUT.println("core " + i + ": average = " + ave);
		res.add("core " + i + ": " + ave);
	}
	OUT.println("---------- RESULT ----------");
	for(s in res) OUT.println(s);
}

