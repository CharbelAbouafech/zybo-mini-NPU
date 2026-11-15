# cracklib renamed its GitHub branch from "master" to "main"; fetch from main instead.
# (PetaLinux 2023.1's cracklib_2.9.8.bb still points at branch=master, so do_fetch fails.)
python () {
    d.setVar('SRC_URI', d.getVar('SRC_URI').replace('branch=master', 'branch=main'))
}
