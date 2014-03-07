utils/url_detector.cpp: utils/url_detector.rl
	ragel $< -T1 -o $@ && sed '1d; /^#line [0-9]/d' -i $@
