# JRI - Java/R Interface      experimental!
#--------------------------------------------------------------------------
#
# *** PLEASE MODIFY SETTINGS BELOW TO MATCH YOUR CONFIGURATION ***
# (later we'll use autoconf, but probably not before the first release ;))

#--- the following settings are OK for Macs
JAVAINC=-I/System/Library/Frameworks/JavaVM.framework/Headers
JNISO=.jnilib
JNILD=-dynamiclib -framework JavaVM
CPICF=-fno-common

#--- the following might work on Linux (if you fix the -L...)
JAVAHOME=/usr/lib/java
JAVAINC=-I$(JAVAHOME)/include -I$(JAVAHOME)/include/linux
JNISO=.so
JNILD=-shared -L$(JAVAHOME)/jre/lib/i386/client -ljvm
CPICF=-fPIC

#--- comment out the following for non-debug version
CFLAGS+=-g

#--- uncomment the one that fits your R installation
#RHOME=/Library/Frameworks/R.framework/Resources
RHOME=/usr/lib/R

#--- if javac is not in the PATH you may want to change the following one
#JAVAB=java
JAVAB=$(JAVAHOME)/bin/java
JAVAC=$(JAVAB)c $(JFLAGS)
JAVAH=$(JAVAB)h

#--------------------------------------------------------------------------
# you shouldn't need to touch anything below this line

RINC=-I$(RHOME)/include
RLD=-L$(RHOME)/bin -lR

TARGETS=libjri$(JNISO) rtest.class run

all: $(TARGETS)

src/Rengine.h: Rengine.class
	$(JAVAH) -d src Rengine

src/Rengine.o: src/Rengine.c src/Rengine.h
	$(CC) -c -o $@ src/Rengine.c $(CFLAGS) $(CPICF) $(JAVAINC) $(RINC)

src/jri.o: src/jri.c
	$(CC) -c -o $@ src/jri.c $(CFLAGS) $(CPICF) $(JAVAINC) $(RINC)

src/jri$(JNISO): src/Rengine.o src/jri.o
	$(CC) -o $@ $^ $(JNILD) $(RLD)

libjri$(JNISO): src/jri$(JNISO)
	ln -sf $^ $@

Rengine.class RXP.class: Rengine.java RXP.java
	$(JAVAC) $^

rtest.class: rtest.java Rengine.class RXP.class
	$(JAVAC) rtest.java

run: libjri$(JNISO) rtest.class
	echo "#!/bin/sh" > run
	echo "export R_HOME=$(RHOME)" >> run
	echo "export DYLD_LIBRARY_PATH=$(RHOME)/bin" >> run
	echo "export LD_LIBRARY_PATH=.:$(RHOME)/bin:$(JAVAHOME)/jre/lib/i386:$(JAVAHOME)/jre/lib/i386/client" >> run
	echo "$(JAVAB) rtest" >> run
	chmod a+x run

clean:
	rm -rf $(TARGETS) src/*.o src/*~ src/Rengine.h src/*$(JNISO) *.class *~

.PHONY: clean all

