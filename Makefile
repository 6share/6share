all :	alclib sdplib nullfeclib xorfeclib rsfeclib rlclib flute

alclib ::
	@echo "-------------------"
	@echo "*** ALC library ***"
	@echo "-------------------"
	mkdir lib; cd alclib; make clean; make                   
	@echo "done"

sdplib ::
	@echo "-------------------"
	@echo "*** SDP library ***"
	@echo "-------------------"
	mkdir lib; cd sdplib; make clean; make                   
	@echo "done"

nullfeclib ::
	@echo "------------------------"
	@echo "*** Null-FEC library ***"
	@echo "------------------------"
	mkdir lib; cd nullfeclib; make clean; make
	@echo "done"

xorfeclib ::
	@echo "------------------------------"
	@echo "*** Simple XOR FEC library ***"
	@echo "------------------------------"
	mkdir lib; cd xorfeclib; make clean; make
	@echo "done"

rsfeclib ::
	@echo "--------------------------------"
	@echo "*** Reed-Solomon FEC library ***"
	@echo "--------------------------------"
	mkdir lib; cd rsfeclib; make clean; make
	@echo "done"

rlclib ::
	@echo "-------------------"
	@echo "*** RLC library ***"
	@echo "-------------------"
	mkdir lib; cd rlclib; make clean; make
	@echo "done"

flute ::
	@echo "-------------------------"
	@echo "*** flute application ***"
	@echo "-------------------------"
	mkdir bin; cd flute; make clean; make
	@echo "done"

clean :
	@echo "----------------------------"
	@echo "*** Cleaning ALC library ***"
	@echo "----------------------------"
	cd alclib; make clean
	@echo "----------------------------"
	@echo "*** Cleaning SDP library ***"
	@echo "----------------------------"
	cd sdplib; make clean
	@echo "---------------------------------"
	@echo "*** Cleaning Null-FEC library ***"
	@echo "---------------------------------"
	cd nullfeclib; make clean
	@echo "---------------------------------------"
	@echo "*** Cleaning Simple XOR FEC library ***"
	@echo "---------------------------------------"
	cd xorfeclib; make clean
	@echo "-----------------------------------------"
	@echo "*** Cleaning Reed-Solomon FEC library ***"
	@echo "-----------------------------------------"
	cd rsfeclib; make clean
	@echo "----------------------------"
	@echo "*** Cleaning RLC library ***"
	@echo "----------------------------"
	cd rlclib; make clean
	@echo "----------------------------------"
	@echo "*** Cleaning flute application ***"
	@echo "----------------------------------"
	cd flute; make clean
	@echo "done"
