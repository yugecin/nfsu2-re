all:
	@[ ! -f "../../NeedForSpeed U2/SPEED2.idc" ] || mv "../../NeedForSpeed U2/SPEED2.idc" ../../nfsu2-re-idc/SPEED2.idc
	bash cmbhashes && gcc -std=c89 -Wall docgen.c -o docgen && ./docgen && rm docgen
