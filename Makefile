#
# Makefile for ITI Recorder
#

CC       ?= gcc
CXX      ?= g++
RM       ?= rm -f
MKDIR    ?= mkdir -p

base_flags = -O3 -g -Wall -Wextra -Wno-unused-parameter

CFLAGS   := ${base_flags} -std=gnu99 ${CFLAGS}
CXXFLAGS := ${base_flags} -std=gnu++17 ${CXXFLAGS}
LIBS     := -lasound

TARGETS := synthrecord test
all: ${TARGETS}

obj := src/.build
src := src

midi_objs := \
	${obj}/Config.o \
	${obj}/Midi.o \
	${obj}/Midi_D5.o \
	${obj}/Midi_DX7.o \
	${obj}/Midi_JU06A.o \
	${obj}/Midi_PSR36.o \
	${obj}/Soundcard.o \

soundcard_objs := \
	${obj}/Soundcard_ALSA.o \

synthrecord_objs := \
	${obj}/synthrecord.o \
	${midi_objs} \
	${soundcard_objs} \

test_objs := \
	${obj}/test.o \
	${midi_objs} \

${test_objs}: | $(filter-out $(wildcard ${obj}), ${obj})
${synthrecord_objs}: | $(filter-out $(wildcard ${obj}), ${obj})

-include ${test_objs:.o=.d}
-include ${synthrecord_objs:.o=.d}

%/.build: %
	${MKDIR} $@

${obj}/%.o: ${src}/%.c
	${CC} -MD ${CFLAGS} -c $< -o $@

${obj}/%.o: ${src}/%.cpp
	${CXX} -MD ${CXXFLAGS} -c $< -o $@

synthrecord: ${synthrecord_objs}
	${CXX} ${LDFLAGS} $^ -o $@ ${LIBS}

test: ${test_objs}
	${CXX} ${LDFLAGS} $^ -o $@ ${LIBS}

clean:
	${RM} ${TARGETS}
	${RM} -r ${obj}
