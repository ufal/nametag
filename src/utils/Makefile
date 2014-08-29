utils/url_detector.cpp: utils/url_detector.rl
	ragel $< -T1 -o $@ && sed '1d; /^#line [0-9]/d; /^static const int [^ ]*_en_main = [0-9]*;$$/d' -i $@
