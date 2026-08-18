INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

TRANSLATIONS += lang/quill_de.ts lang/quill_ru.ts \
                lang/quill_es.ts lang/quill_fr.ts lang/quill_hu.ts \
                lang/quill_sv.ts lang/quill_sr.ts lang/quill_nl.ts \
                lang/quill_fa.ts lang/quill_it.ts lang/quill_zh_CN.ts \
                lang/quill_uk.ts lang/quill_cs.ts lang/quill_pl.ts \
                lang/quill_ja.ts lang/quill_ko.ts lang/quill_pt_BR.ts \
                lang/quill_lt.ts lang/quill_zh_TW.ts lang/quill_el_GR.ts \
                lang/quill_tr.ts lang/quill_ar.ts lang/quill_sk.ts \
                lang/quill_tg_TJ.ts lang/quill_pt_PT.ts lang/quill_vi.ts \
                lang/quill_ro_RO.ts lang/quill_fi.ts lang/quill_gl.ts \
                lang/quill_bg.ts lang/quill_hi.ts

isEmpty(QMAKE_LRELEASE) {
  Q_OS_WIN:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\lrelease.exe
  else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
}

updateqm.input = TRANSLATIONS
updateqm.output = $$DESTDIR/lang/${QMAKE_FILE_BASE}.qm
updateqm.commands = $$QMAKE_LRELEASE \"${QMAKE_FILE_IN}\" -qm \"$$DESTDIR/lang/${QMAKE_FILE_BASE}.qm\"
updateqm.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += updateqm
