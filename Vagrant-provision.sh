#!/usr/bin/env bash

add-apt-repository "deb http://archive.ubuntu.com/ubuntu $(lsb_release -sc) universe"
apt-get update
apt-get install -y build-essential libboost-mpi-dev
ln -s /vagrant /home/vagrant/code
chown vagrant:vagrant /home/vagrant/code

# --------------------- Preferences -------------------------
#
# These items are nice for IDE and development, but
# are not required for working with the code
#
# -----------------------------------------------------------
apt-get install -y git vim screen git

# Setup gitconfig
git clone https://github.com/hamiltont/dotfiles.git /home/vagrant/.dotfiles
cp /home/vagrant/.dotfiles/.gitconfig /home/vagrant/.gitconfig

# Merge .hostssh and .ssh folders so we have all of the private keys available
# Only works because 1) we sync host ~/.ssh to VM ~/.hostssh and 2) vagrant sets 
# up the synced folders before it runs the shell provider
cp -R /home/vagrant/.hostssh /tmp/.hostssh
cat /home/vagrant/.ssh/authorized_keys >> /tmp/.hostssh/authorized_keys
mv /tmp/.hostssh/* /home/vagrant/.ssh

# Setup vim plugins
apt-get install -y curl
mkdir -p /home/vagrant/.vim/autoload /home/vagrant/.vim/bundle
curl -Sso /home/vagrant/.vim/autoload/pathogen.vim https://raw.github.com/tpope/vim-pathogen/master/autoload/pathogen.vim

# VIM - Pathogen
echo "\" Setup Pathogen
execute pathogen#infect()
call pathogen#helptags()
syntax on
filetype plugin indent on" > /home/vagrant/.vimrc

# VIM - NERDTree
git clone https://github.com/scrooloose/nerdtree.git /home/vagrant/.vim/bundle/nerdtree

# VIM - TagBar
apt-get install -y exuberant-ctags
git clone https://github.com/majutsushi/tagbar.git /home/vagrant/.vim/bundle/tagbar
echo "nmap <F8> :TagbarToggle<CR>" >> /home/vagrant/.vimrc


# TODO Run Pathogen's version of helptags once to generate documentation
# vim :Helptags


# Setup some personal/project-specific vim settings
echo "
set tabstop=4     \" a tab is four spaces
set autoindent    \" always set autoindenting on
set copyindent    \" copy the previous indentation on autoindenting
set number        \" always show line numbers
set shiftwidth=4  \" number of spaces to use for autoindenting
set shiftround    \" use multiple of shiftwidth when indenting with '<' and '>'
set showmatch     \" set show matching parenthesis
set hlsearch      \" highlight search terms
set incsearch     \" show search matches as you type
set nobackup      \" I hate swp files
set noswapfile
set pastetoggle=<F2>   \" press f2 to enable pasting without smart indention
set mouse=a       \" enable the mouse" >> /home/vagrant/.vimrc