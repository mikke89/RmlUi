#pragma once

#include <RmlUi/Core/Types.h>
#include <algorithm>

class TestSuite {
public:
	enum class Direction { None, Forward, Backward, Any };

	TestSuite(Rml::String directory, Rml::StringList files) : directory(std::move(directory)), files(std::move(files))
	{
		RMLUI_ASSERTMSG(!this->files.empty(), "At least one file in the test suite is required.");
	}

	const Rml::String& GetDirectory() const { return directory; }
	const Rml::String& GetFilename() const { return files[index]; }
	Rml::String GetPath() const { return directory + '/' + files[index]; }

	const Rml::String& GetFilter() const { return filter; }
	void SetFilter(Rml::String new_filter)
	{
		filter = new_filter;
		UpdateFilteredTests();
		SetIndex(index, Direction::Any);
	}

	bool SetIndex(int new_index, Direction filter_direction = Direction::None)
	{
		new_index = GetIndexFiltered(new_index, filter_direction);
		if (new_index < 0 || new_index >= (int)files.size())
			return false;
		index = new_index;
		return true;
	}

	bool Next() { return SetIndex(index + 1, Direction::Forward); }
	bool Previous() { return SetIndex(index - 1, Direction::Backward); }

	int GetIndex() const { return index; }

	int GetFilterIndex() const
	{
		auto it = std::lower_bound(filtered_tests_indices.begin(), filtered_tests_indices.end(), index);
		if (it == filtered_tests_indices.end() || *it != index)
			return -1;
		return int(it - filtered_tests_indices.begin());
	}

	int GetNumTests() const { return (int)files.size(); }

	int GetNumFilteredTests() const { return filter.empty() ? GetNumTests() : (int)filtered_tests_indices.size(); }

private:
	int GetIndexFiltered(int new_index, Direction filter_direction) const
	{
		if (filter_direction == Direction::None || filtered_tests_indices.empty())
			return new_index;

		auto it = std::lower_bound(filtered_tests_indices.begin(), filtered_tests_indices.end(), new_index);

		// Exact match
		if (it != filtered_tests_indices.end() && *it == new_index)
			return *it;

		if (filter_direction == Direction::Forward)
		{
			if (it != filtered_tests_indices.end())
				return *it;
		}
		else if (filter_direction == Direction::Backward)
		{
			if (it != filtered_tests_indices.begin())
				return *(it - 1);
		}
		else if (filter_direction == Direction::Any)
		{
			// Like forward but will go back if we cannot go forward.
			return it == filtered_tests_indices.end() ? *(it - 1) : *it;
		}

		return -1;
	}

	bool MatchesFilter(int id) const
	{
		RMLUI_ASSERT(id >= 0 && id < (int)files.size());
		return (files[id].find(filter) != Rml::String::npos);
	}

	void UpdateFilteredTests()
	{
		filtered_tests_indices.clear();
		for (int i = 0; i < (int)files.size(); i++)
		{
			if (MatchesFilter(i))
				filtered_tests_indices.push_back(i);
		}
	}

	Rml::String directory;
	Rml::StringList files;

	int index = 0;

	Rml::String filter;

	Rml::Vector<int> filtered_tests_indices;
};

using TestSuiteList = Rml::Vector<TestSuite>;
