# sphinx
开源文档使用sphinx编译，并放在read the docs上托管。

## 本地使用
文档源代码以及生成的html存放在docs_spinx目录下。在文档的目录下`make html`编译出hetml，在html的目录中运行`python -m http.server 8000`查看本地html.

## read the docs平台
read the docs编译需要requirements.txt和.readthedocs.yaml文件。并且与github仓库绑定，上传的代码会实时更新read the docs.

## 文档结构
文档的内容都放在docs_sphinx下的source下。主目录的index.rst是首页，目前主要是承担目录的作用。

conf.py是sphinx的配置文件