![](https://raw.githubusercontent.com/doveC/AlphaJsonParser/master/resources/AlphaJsonParser.png)

# 使用C++17标准的JSON解析器

- 使用std::variant管理json数据类型，std::string_view进行代理模式明确只读语义，std::optional处理非侵入式异常处理机制
- 通过递归下降对json字符串进行解析，解析后支持动态修改，支持格式化输出json结构，接口设计便捷，使用方便
