GENERATORS = ../randomSeq ../almostSortedSeq ../exptSeq ../trigramSeq ../addDataSeq ../trigramString

STRINGFILES = wikisamp.xml chr22.dna etext99
STRINGFILES_LONG = wikisamp.xml chr22.dna etext99 HG18 howto jdk13c proteins rctail96 rfc sprot34 w3c2

$(GENERATORS) :
	cd ..; make -s $(subst ../,,$@)

$(STRINGFILES) : $(STRINGBZIP)
	bunzip2 -k -f $@.bz2

$(STRINGBZIP) :
	cp ../../data/$@ .

%_pair_int :  ../addDataSeq
	make -s $(subst _pair_int,,$@)
	../addDataSeq -t int $(subst _pair_int,,$@) $@

randomSeq_100_% : ../randomSeq
	../randomSeq -t $(subst randomSeq_100_,,$@) 100 $@

randomSeq_10M_% : ../randomSeq
	../randomSeq -t $(subst randomSeq_10M_,,$@) 10000000 $@

randomSeq_100M_% : ../randomSeq
	../randomSeq -t $(subst randomSeq_100M_,,$@) 100000000 $@

randomSeq_% : ../randomSeq
	../randomSeq -t int $(subst randomSeq_,,$@) $@

# In this one the integers are limited to a range of 100K
randomSeq_10M_100K_int : ../randomSeq
	../randomSeq -t int -r 100000 10000000 $@

randomSeq_10M_256_int : ../randomSeq
	../randomSeq -t int -r 256 10000000 $@

randomSeq_100M_256_int : ../randomSeq
	../randomSeq -t int -r 256 100000000 $@

exptSeq_10M_% : ../exptSeq
	../exptSeq -t $(subst exptSeq_10M_,,$@) 10000000 $@

exptSeq_100M_% : ../exptSeq
	../exptSeq -t $(subst exptSeq_100M_,,$@) 100000000 $@

almostSortedSeq_10M_% : ../almostSortedSeq
	../almostSortedSeq -t $(subst almostSortedSeq_10M_,,$@) 10000000 $@

almostSortedSeq_100M_% : ../almostSortedSeq
	../almostSortedSeq -t $(subst almostSortedSeq_100M_,,$@) 100000000 $@

trigramSeq_10M : ../trigramSeq
	../trigramSeq 10000000 $@

trigramSeq_100M : ../trigramSeq
	../trigramSeq 100000000 $@

trigramSeq_% : ../trigramSeq
	../trigramSeq $(subst trigramSeq_,,$@) $@

trigramString_% : ../trigramString
	../trigramString $(subst trigramString_,,$@) $@

clean :
	rm -f *0* $(STRINGFILES)
