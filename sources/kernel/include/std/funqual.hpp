#pragma once

#define QTAG(id) __attribute__((annotate("funqual::" #id)))
#define QTAG_INDIRECT(id) __attribute__((annotate("funqual_indirect::" #id)))

#define NON_REENTRANT QTAG(non_reentrant)
#define REENTRANT QTAG(reentrant)
#define SIGNAL_HANDLER QTAG(signal_handler)
