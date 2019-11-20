# Simple Serializer

This project is a very simple to use serializer to map C++ types to either YAML or EBML.
Here is a minimalistic example for serialization:

~~~C++
	serializer::ebml::Serializer serializer;

	std::map<std::string, std::string> map {{"hello", "world"}, {"bla", "fasel"}};
	std::vector<int> vec {1, 2, 3};
	int myInt = 123;
	
	serializer["map"]          % map;
	serializer["vec"]          % vec;
	serializer["some_integer"] % myInt;
~~~

Here is the correspondinc counterpart:

~~~C++
	serializer::ebml::Deserializer deserializer;

	std::map<std::string, std::string> map;
	std::vector<int> vec;
	int myInt;
	
	deserializer["map"]          % map;
	deserializer["vec"]          % vec;
	deserializer["some_integer"] % myInt;
~~~

Find a more elaborate runnable example in the "example" branch.