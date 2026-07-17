DST_PLATFORM := QVM

# QuakeC source directory for this mod. Swap this (or override QC_DIR=...
# on the command line) to point at a different GamesQC/* tree later.
QC_DIR ?= ../src/game/GamesQC/v101qc

# Flat list of .qc files, order matters (mirrors the original progs.src).
# No INCLUDES here -- QuakeC has no header/include mechanism.
QC_FILES += $(QC_DIR)/defs.qc
QC_FILES += $(QC_DIR)/subs.qc
QC_FILES += $(QC_DIR)/fight.qc
QC_FILES += $(QC_DIR)/ai.qc
QC_FILES += $(QC_DIR)/combat.qc
QC_FILES += $(QC_DIR)/items.qc
QC_FILES += $(QC_DIR)/weapons.qc
QC_FILES += $(QC_DIR)/world.qc
QC_FILES += $(QC_DIR)/client.qc
QC_FILES += $(QC_DIR)/player.qc
QC_FILES += $(QC_DIR)/monsters.qc
QC_FILES += $(QC_DIR)/doors.qc
QC_FILES += $(QC_DIR)/buttons.qc
QC_FILES += $(QC_DIR)/triggers.qc
QC_FILES += $(QC_DIR)/plats.qc
QC_FILES += $(QC_DIR)/misc.qc
QC_FILES += $(QC_DIR)/ogre.qc
QC_FILES += $(QC_DIR)/demon.qc
QC_FILES += $(QC_DIR)/shambler.qc
QC_FILES += $(QC_DIR)/knight.qc
QC_FILES += $(QC_DIR)/soldier.qc
QC_FILES += $(QC_DIR)/wizard.qc
QC_FILES += $(QC_DIR)/dog.qc
QC_FILES += $(QC_DIR)/zombie.qc
QC_FILES += $(QC_DIR)/boss.qc
QC_FILES += $(QC_DIR)/tarbaby.qc
QC_FILES += $(QC_DIR)/hknight.qc
QC_FILES += $(QC_DIR)/fish.qc
QC_FILES += $(QC_DIR)/shalrath.qc
QC_FILES += $(QC_DIR)/enforcer.qc
QC_FILES += $(QC_DIR)/oldone.qc
