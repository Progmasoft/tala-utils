# SPDX-FileCopyrightText: 2026 Progmasoft <support@progmasoft.com>
# SPDX-License-Identifier: MIT
#
# Builds Tala Utils with BSD Make (bmake on Linux and macOS).
#
#   make            build/libcommon.a, the code the utilities share
#   make test       the tests (SANITIZE=-fsanitize=address,undefined adds the sanitizers)
#   make format     checks the layout with clang-format 23
#
# Every executable of the repository has the one version in VERSION; the build gives it to the code as
# TALAUTILS_VERSION.

CC?=		clang
AR?=		ar
CLANG_FORMAT?=	clang-format
SANITIZE?=

BUILD=		build
VERSION!=	cat VERSION
LIBRARY=	${BUILD}/libcommon.a
TESTS=		${BUILD}/tests/tala-utils-tests

WARNINGS=	-std=c11 -pedantic-errors -Wall -Wextra -Werror
ALL_CFLAGS=	${WARNINGS} -O2 -g ${SANITIZE} -Icommon -DTALAUTILS_VERSION=\"${VERSION}\"
TEST_CFLAGS=	${ALL_CFLAGS} -Itests

COMMON_SRCS!=	ls common/*.c
TEST_SRCS!=	ls tests/*.c
HDRS!=		ls common/*.h tests/*.h

all: ${LIBRARY}

COMMON_OBJS=
.for src in ${COMMON_SRCS}
COMMON_OBJS+=	${BUILD}/${src:R}.o
${BUILD}/${src:R}.o: ${src} ${HDRS} VERSION
	@mkdir -p ${.TARGET:H}
	${CC} ${ALL_CFLAGS} -c ${src} -o ${.TARGET}
.endfor

TEST_OBJS=
.for src in ${TEST_SRCS}
TEST_OBJS+=	${BUILD}/${src:R}.o
${BUILD}/${src:R}.o: ${src} ${HDRS}
	@mkdir -p ${.TARGET:H}
	${CC} ${TEST_CFLAGS} -c ${src} -o ${.TARGET}
.endfor

${LIBRARY}: ${COMMON_OBJS}
	rm -f ${.TARGET}
	${AR} rcs ${.TARGET} ${COMMON_OBJS}

${TESTS}: ${TEST_OBJS} ${LIBRARY}
	${CC} ${SANITIZE} -o ${.TARGET} ${TEST_OBJS} ${LIBRARY}

test: ${TESTS}
	${TESTS}

format:
	${CLANG_FORMAT} --style=file --dry-run --Werror ${COMMON_SRCS} ${TEST_SRCS} ${HDRS}

clean:
	rm -rf ${BUILD}

.PHONY: all test format clean
