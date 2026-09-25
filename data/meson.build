subdir('icons')

qthash_desktop = files('dev.kingbeencent.qthash.desktop')
install_data(
    qthash_desktop,
    install_dir: get_option('datadir') / 'applications',
)

desktop_file_validate = find_program('desktop-file-validate', required: false)
if desktop_file_validate.found()
    test('Validate qthash desktop file', desktop_file_validate,
        args: [qthash_desktop])
endif
