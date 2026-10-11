#pragma once
#include<fstream>
#include<vector>
#include<stdexcept>

namespace de::io {

	template <typename T>
	void writeVector(std::ofstream& out, const std::vector<T>& v) {
		const uint64_t count = v.size();
		out.write(reinterpret_cast<const char*>(&count), sizeof(count));
		if (count > 0) {
			out.write(reinterpret_cast<const char*>(v.data()),
				static_cast<std::streamsize>(count * sizeof(T)));
		}
	}

	template <typename T>
	std::vector<T> readVector(std::ifstream& in) {
		uint64_t count{};
		in.read(reinterpret_cast<char*>(&count), sizeof(count));
		if (!in) { throw std::runtime_error("truncated file (count)"); }

		std::vector<T> v(static_cast<size_t>(count));
		if (count > 0) {
			in.read(reinterpret_cast<char*>(v.data()),
				static_cast<std::streamsize>(count * sizeof(T)));

			if(!in) { throw std::runtime_error("truncated file (data)"); }
		}
		return v;
	}
}