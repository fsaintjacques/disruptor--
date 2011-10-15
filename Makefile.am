AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}


disruptordir = $(includedir)/disruptor
disruptor_HEADERS = disruptor/batch_descriptor.h \
					disruptor/claim_strategy.h \
				    disruptor/event_processor.h \
				    disruptor/event_publisher.h \
				    disruptor/exceptions.h \
				    disruptor/interface.h \
				    disruptor/ring_buffer.h \
				    disruptor/sequence.h \
				    disruptor/sequence_barrier.h \
				    disruptor/sequencer.h \
				    disruptor/utils.h \
				    disruptor/wait_strategy.h

SUBDIRS = test perf
