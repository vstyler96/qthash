# Scalable only: Plasma renders the SVG at every size, and every installed file
# stays in meson's install log so `make uninstall` removes it.
install_data(
    'scalable/@0@.svg'.format(application_id),
    install_dir: get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
)
