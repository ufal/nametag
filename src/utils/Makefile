all: $(patsubst %.rl,%.cpp,$(wildcard *.rl))

%.cpp: %.rl
	ragel $< -T1 -o $@ && sed '1d; /^#line [0-9]/d' -i $@

%.eps: %.rl
	ragel $< -pV | dot -Teps >$@

.PHONY: clean
clean:
	rm -rf *.eps
