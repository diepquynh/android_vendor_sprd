
#NOTICE: Assets目录下语言包文件命名规范

*目的：* 为了更好的适配程序中对于语言的包解析的相关配置，我们必须使用以下规则命名语言包：

## langpack语言包的命名
> "langpack" + \[index[见注意事项]\] + [语言简写] + [语言包更新时间] + [版本号].zip

例如：langpack_1_zh_20150222_1.0.zip

## trigger语言包的命名
> "trigger" + \[index[见注意事项]\]+ [语言简写] + [语言包更新时间] + [版本号].zip

例如：trigger_1_en-us_20150222_1.0.zip

## 注意事项
> 由于asset的解包顺序影响到了语言包加入到arraylist的顺序，进而影响到了和语言列表的一一对应关系，
所以必须对包名称进行index的声明，顺序需要和 values/arrays.xml中support_languages_key字段中的语言顺序一致。