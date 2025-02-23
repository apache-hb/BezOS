#pragma once

#define QTAG(id) __attribute__((annotate("funqual::" #id)))
#define QTAG_INDIRECT(id) __attribute__((annotate("funqual_indirect::" #id)))

#define QTAG_NON_REENTRANT QTAG(non_reentrant)
#define QTAG_INTERRUPT QTAG(interrupt)
