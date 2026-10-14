CXX = mpicxx

CXXFLAGS = -Wall -O2

PASCAL_LIBS = -lmpascalops

TARGETS = RB_mpi RB_mpi_instr RB_mpi_instr_otmzd

.PHONY: all clean

all: $(TARGETS)

# 1. Versão inicial (sem instrumentação manual)
RB_mpi: rb.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# 2. Versão com instrumentação manual (pré-correção)
RB_mpi_instr: rb_instr.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(PASCAL_LIBS)

# 3. Versão instrumentada e otimizada (pós-correção)
RB_mpi_instr_otmzd: rb_instr_otmzd.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(PASCAL_LIBS)

# Remove os binários compilados
clean:
	rm -f $(TARGETS)
