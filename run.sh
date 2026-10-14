#!/bin/bash
#SBATCH --job-name=minicurso_pascal
#SBATCH --nodes=8
#SBATCH --ntasks-per-node=1
#SBATCH --time=48:00:00        
#SBATCH --output=./saida/minicurso-pascal-%j.out
#SBATCH --mem=0           
#SBATCH --exclusive   
#SBATCH --partition=intel-128

mkdir ./saida
module purge
module load libraries/openmpi/4.1.1-gnu8
source /home/fhsdsilva/testes-sidekick-lm/pascalsuite-analyzer/venv/bin/activate

RB1=./RB_mpi
RB2=./RB_mpi_instr
RB3=./RB_mpi_instr_otmzd
REPETITIONS=5

## ! RB sem instrumentação
pascalanalyzer "$RB1" \
    --ranks 1,2,4,8 \
    -r $REPETITIONS \
    -i  1001,1501,2001,2501 \
    --mpi srun \
    -o rb_mpi_${SLURM_JOB_ID}.json

## ! RB com instrumentação
pascalanalyzer "$RB2" \
  --ranks 1,2,4,8 \
  -r $REPETITIONS \
  -i  1001,1501,2001,2501 \
  -t man \
  --mpi srun \
  -o rb_mpi_${SLURM_JOB_ID}_instr.json

## ! RB com instrumentação + otimizado
pascalanalyzer "$RB3" \
  --ranks 1,2,4,8 \
  -r $REPETITIONS \
  -i  1001,1501,2001,2501 \
  -t man \
  --mpi srun \
  -o rb_mpi_${SLURM_JOB_ID}_instr_otmzd.json

