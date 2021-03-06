# -----------------------------------------------------------------------------
#	Borland [tb]cc makefile for compiling and testing the UGST G.711 
#	implementation.
#	Implemented by <simao@cpqd.ansp.br> -- 01.Feb.95
# -----------------------------------------------------------------------------

# ------------------------------------------------
# Choose a file comparison utility: 
# ------------------------------------------------
DIFF = cf -q

# ------------------------------------------------
# Choose an archiving utility: 
#	- public domain unzip, or [PC/Unix/VMS]
#	- shareware pkunzip [PC only]
# ------------------------------------------------
#UNZIP = -pkunzip
UNZIP = unzip -o

# ------------------------------------------------
# Choose an AWK; suggest use GNU version 
#                (available via anonymous ftp) 
# ------------------------------------------------
AWK = gawk
###AWK_CMD = '$$6~/[0-9]+:[0-9][0-9]/ {print "sb -over",$$NF};END{print "exit"}'
AWK_CMD = '$6~/[0-9]+:[0-9][0-9]/{{print "sb -over",$NF;print "touch",$NF}}'

# ------------------------------------------------
# Choose compiler. Sun: use cc. HP: gotta use gcc
# ------------------------------------------------
CC = tcc
CC_OPT = -I../utl

# ------------------------------------------------
# General purpose symbols
# ------------------------------------------------
RM = -rm -f
REF_VECTORS= sweep-r.a sweep-r.a-a sweep-r.rea sweep-r.reu \
	sweep-r.u sweep-r.u-u sweep.src 
TEST_VECTORS = sweep.a sweep.a-a sweep.rea sweep.reu sweep.u sweep.u-u
G711_OBJ = g711.obj g711demo.obj
G711DEMO = g711demo

# ------------------------------------------------
# Generic rules
# ------------------------------------------------
.c.obj:
	$(CC) $(CC_OPT) -c $<

# ------------------------------------------------
# Targets
# ------------------------------------------------
all: g711demo

anyway: clean g711demo

clean:
	$(RM) $(G711_OBJ)

cleantest:
	$(RM) $(TEST_VECTORS)
	$(RM) $(REF_VECTORS) 

veryclean: clean cleantest
	$(RM) g711demo.exe

# ------------------------------------------------
# Specific rules
# ------------------------------------------------
g711demo:	g711demo.exe
g711demo.exe:	g711demo.obj g711.obj
	$(CC) $(CC_OPT) -eg711demo g711demo.obj g711.obj

shiftbit:	shiftbit.exe
shiftbit.exe:	shiftbit.c 
	$(CC) $(CC_OPT) -eshiftbit shiftbit.c

# ------------------------------------------------
# Test portability
# Note: there are no compliance test vectors associated with the G711 module
# ------------------------------------------------
test: proc comp

# sweep-r.* have been generated from sweep.src in a reference environment
# results of the comparisons shall yield 0 different samples!
proc:	sweep.src
	$(G711DEMO) A lilo sweep.src sweep.a 256 1 256
	$(G711DEMO) u lilo sweep.src sweep.u 256 1 256
	$(G711DEMO) A lili sweep.src sweep.a-a 256 1 256
	$(G711DEMO) u lili sweep.src sweep.u-u 256 1 256
	$(G711DEMO) A loli sweep.a sweep.rea 256 1 256
	$(G711DEMO) u loli sweep.u sweep.reu 256 1 256

comp: 	sweep-r.u-u
	$(DIFF) sweep.a   sweep-r.a 
	$(DIFF) sweep.a-a sweep-r.a-a 
	$(DIFF) sweep.rea sweep-r.rea 
	$(DIFF) sweep.reu sweep-r.reu 
	$(DIFF) sweep.u   sweep-r.u 
	$(DIFF) sweep.u-u sweep-r.u-u 

# ---------------------------------------------------
# Extract from archive and byte-swap -- if necessary
# ---------------------------------------------------
sweep.src:	tst-g711.zip
	$(UNZIP) tst-g711.zip sweep.src 
	swapover sweep.src
#	$(UNZIP) -v tst-g711.zip sweep.src > x
#	$(AWK) $(AWK_CMD) x > y.bat
#	y
#	$(RM) x y.bat

sweep-r.u-u:	tst-g711.zip
	$(UNZIP) tst-g711.zip $(REF_VECTORS) 
	swapover $(REF_VECTORS) 
#	$(UNZIP) -v tst-g711.zip sweep-r.* > x
#	$(AWK) $(AWK_CMD) x > y.bat
#	y
#	$(RM) x y.bat
