GIT_BRANCH      := $(shell git rev-parse --abbrev-ref HEAD)
GIT_COMMIT_HASH := $(shell git rev-parse HEAD)
# $(info GIT_BRANCH:$(GIT_BRANCH) GIT_COMMIT_HASH:$(GIT_COMMIT_HASH))
GIT_INFO        = -DGIT_BRANCH=\"$(GIT_BRANCH)\" -DGIT_COMMIT_HASH=\"$(GIT_COMMIT_HASH)\"