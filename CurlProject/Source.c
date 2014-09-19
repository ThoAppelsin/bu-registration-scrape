#define HAS_QUOTA_COLUMN

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <curl/curl.h>
#include <time.h>

#define BASE_LINK						L"http://registration.boun.edu.tr"
#define SEMESTER_SELECTION_PAGE_LINK	BASE_LINK L"/schedule.htm"

const wchar_t BaseLink[ ]					= BASE_LINK;
const wchar_t SemesterSelectionPageLink[ ]	= SEMESTER_SELECTION_PAGE_LINK;

typedef struct ColumnIndexLookupTag
{
	size_t CodeSec;
	size_t Name;
	size_t Cr;
	size_t Instr;
	size_t Days;
	size_t Hours;
	size_t Rooms;
} ColumnIndexLookup;

typedef struct MyLookupTag
{
	wchar_t * Entry;
	size_t OrderedIndex;
} MyLookup;

typedef struct PhraseMatchInAmbiguityTag
{
	size_t NumberofWordsPassed;
	MyLookup * PLookupForMatch;
} PhraseMatchInAmbiguity;

typedef enum
{
	Monday = 1,
	Tuesday,
	Wednesday,
	Thursday,
	Friday,
	Saturday,
	Sunday
} DAYS;

typedef struct ExtremesTag
{
	DAYS FirstDay;
	DAYS LastDay;
	unsigned int FirstSlot;
	unsigned int LastSlot;
	size_t LongestProgrammeEntryLength;
	size_t LongestLessonEntryLength;
	size_t LongestLessonNameEntryLength;
	size_t LongestCreditsEntryLength;
	size_t LongestLinkEntryLength;
	size_t LongestLectureTypeEntryLength;
	size_t LongestSectionEntryLength;
	size_t LongestInstructorEntryLength;
	size_t LongestRawDaysEntryLength;
	size_t LongestRawSlotsEntryLength;
	size_t LongestRawClassroomsEntryLength;
	size_t LongestClassroomEntryLength;
} Extremes;

typedef struct MyLectureTag
{
	DAYS Day;
	unsigned int Slot;
	MyLookup * PClassroomLookup;
} MyLecture;

typedef enum MyLectureGroupStateTag
{
	LectureGroupState_Undefined,
	LectureGroupState_TBA,
	LectureGroupState_Failed,
	LectureGroupState_OnlyDays,
	LectureGroupState_DaysandSlots,
	LectureGroupState_Complete
} MyLectureGroupState;

typedef struct MyLectureGroupTag
{
	MyLookup * PSectionLookup;
	MyLookup * PInstructorLookup;
	MyLookup * PRawDaysLookup;
	MyLookup * PRawSlotsLookup;
	MyLookup * PRawClassroomsLookup;
	MyLectureGroupState LectureGroupState;
	size_t NumberofLectures;
	MyLecture * ALectures;
} MyLectureGroup;

typedef struct MyLectureGroupTypeTag
{
	MyLookup * PLectureTypeLookup;
	MyLectureGroup ** APLectureGroups;
} MyLectureType;

typedef struct MyLessonTag
{
	MyLookup * PLessonLookup;
	MyLookup * PLessonNameLookup;
	MyLookup * PCreditsLookup;
	MyLookup * PLinkLookup;
	MyLectureType ** APLectureTypes;
} MyLesson;

typedef struct MyProgrammeTag
{
	MyLookup * PProgrammeLookup;
	MyLesson ** APLessons;
} MyProgramme;

MyLookup ** APProgrammeLookups = NULL;
MyLookup ** APLessonLookups = NULL;
MyLookup ** APLessonNameLookups = NULL;
MyLookup ** APCreditsLookups = NULL;
MyLookup ** APLinkLookups = NULL;
MyLookup ** APLectureTypeLookups = NULL;
MyLookup ** APSectionLookups = NULL;
MyLookup ** APInstructorLookups = NULL;
MyLookup ** APRawDaysLookups = NULL;
MyLookup ** APRawSlotsLookups = NULL;
MyLookup ** APRawClassroomsLookups = NULL;
MyLookup ** APClassroomLookups = NULL;

MyProgramme ** APProgrammes = NULL;

typedef struct
{
	size_t count;
	wchar_t * memory;
} MemoryStruct;

int WcsIsWcsWithStartAndEnd(const wchar_t * wcs1, const wchar_t * wcs2_start, const wchar_t * wcs2_end)
{
	while (*wcs1 && wcs2_start < wcs2_end && *wcs1 == *wcs2_start)
	{
		wcs1++;
		wcs2_start++;
	}
	return (*wcs1 == 0 && wcs2_start == wcs2_end) ? 1 : 0;
}

int WcsFoundAtWcsWithStartAndEnd(const wchar_t * wcs1, const wchar_t * wcs2_start, const wchar_t * wcs2_end)
{
	while (*wcs1 && wcs2_start < wcs2_end && *wcs1 == *wcs2_start)
	{
		wcs1++;
		wcs2_start++;
	}
	return (*wcs1 == 0) ? 1 : 0;
}

const wchar_t * wcswithstartandendstr(const wchar_t * start, const wchar_t * end, const wchar_t * const SubStr)
{
	const size_t SearchLength = (end - start) - wcslen(SubStr) + 1;
	for (size_t i = 0; i < SearchLength; i++)
	{
		for (size_t j = 0; start[j] == SubStr[j]; j++)
		{
			if (SubStr[j + 1] == L'\0')
			{
				return start;
			}
		}
		start++;
	}
	return NULL;
}

void wcscpywithstartandend(wchar_t * const Destinationwcs, const wchar_t * const Start, const wchar_t * const End)
{
	size_t Length = End - Start;
	wcsncpy(Destinationwcs, Start, Length);
	Destinationwcs[Length] = 0;
}

wchar_t * wcsdupwithstartandend(const wchar_t * const Start, const wchar_t * const End)
{
	wchar_t * TheDupe = malloc((End - Start + 1) * sizeof * TheDupe);
	if (TheDupe == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	wcscpywithstartandend(TheDupe, Start, End);
	return TheDupe;
}

const wchar_t * wcsstradvance(const wchar_t * const wcs, const wchar_t * const substr)
{
	const wchar_t * const result = wcsstr(wcs, substr);
	if (result)
		return result + wcslen(substr);
	else
		return result;
}

// LeftLimitwcs has to be within SourceStart
// and come before RightLimitwcs (also within SourceStart)
// else function will return NULL
const wchar_t * MatchFromRightToLeft(const wchar_t * SourceStart, const wchar_t * const LeftLimitwcs, const wchar_t * const RightLimitwcs,
									 const wchar_t ** const PMatchStart, const wchar_t ** const PMatchEnd)
{
	*PMatchEnd = wcsstr(SourceStart, RightLimitwcs);
	if (*PMatchEnd == NULL)
	{
		return NULL;
	}

	SourceStart = wcsstr(SourceStart, LeftLimitwcs);
	if (SourceStart == NULL)
	{
		return NULL;
	}
	while (SourceStart && SourceStart < *PMatchEnd)
	{
		*PMatchStart = SourceStart;
		SourceStart = wcsstr(SourceStart + 1, LeftLimitwcs);
	}
	*PMatchStart += wcslen(LeftLimitwcs);

	return *PMatchEnd + wcslen(RightLimitwcs);
}

const wchar_t * MatchFromLeftToRight(const wchar_t * const SourceStart, const wchar_t * const LeftLimitwcs, const wchar_t * const RightLimitwcs,
									 const wchar_t ** const PMatchStart, const wchar_t ** const PMatchEnd)
{
	*PMatchStart = wcsstr(SourceStart, LeftLimitwcs);
	if (*PMatchStart == NULL)
	{
		return NULL;
	}
	*PMatchStart += +wcslen(LeftLimitwcs);

	*PMatchEnd = wcsstr(*PMatchStart, RightLimitwcs);
	if (*PMatchEnd == NULL)
	{
		return NULL;
	}
	
	return *PMatchEnd + wcslen(RightLimitwcs);
}

void InitializeLectureGroup(MyLectureGroup * const PLectureGroup)
{
	PLectureGroup->PSectionLookup = APSectionLookups[0];
	PLectureGroup->PInstructorLookup = APInstructorLookups[0];
	PLectureGroup->LectureGroupState = LectureGroupState_Undefined;
	PLectureGroup->PRawDaysLookup = APRawDaysLookups[0];
	PLectureGroup->PRawSlotsLookup = APRawSlotsLookups[0];
	PLectureGroup->PRawClassroomsLookup = APRawClassroomsLookups[0];
	PLectureGroup->NumberofLectures = 0U;
	PLectureGroup->ALectures = NULL;
}

MyLectureGroup * OneMoreLectureGroup(MyLectureGroup *** const PAPLectureGroups)
{
	size_t size = 0;
	while ((*PAPLectureGroups)[size] != NULL)
		size++;

	(*PAPLectureGroups) = realloc((*PAPLectureGroups), (size + 2) * sizeof * (*PAPLectureGroups));
	if ((*PAPLectureGroups) == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	(*PAPLectureGroups)[size] = malloc(sizeof * (*PAPLectureGroups)[size]);
	if ((*PAPLectureGroups)[size] == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	(*PAPLectureGroups)[size + 1] = NULL;

	InitializeLectureGroup((*PAPLectureGroups)[size]);

	return (*PAPLectureGroups)[size];
}

void InitializeLectureType(MyLectureType * const PLectureType)
{
	PLectureType->PLectureTypeLookup = APLectureTypeLookups[0];

	PLectureType->APLectureGroups = malloc(sizeof * PLectureType->APLectureGroups);
	PLectureType->APLectureGroups[0] = NULL;
}

void InitializeLesson(MyLesson * const PLesson)
{
	PLesson->PLessonLookup = APLessonLookups[0];
	PLesson->PLessonNameLookup = APLessonNameLookups[0];
	PLesson->PCreditsLookup = APCreditsLookups[0];
	PLesson->PLinkLookup = APLinkLookups[0];

	// allocate two space for two pointers, first to be filled with a pointer to space for a MyLectureType
	PLesson->APLectureTypes = malloc(2 * sizeof * PLesson->APLectureTypes);
	PLesson->APLectureTypes[1] = NULL;

	// fetch a MyLectureType and initialize (a MyLesson will definitely have one)
	PLesson->APLectureTypes[0] = malloc(sizeof * PLesson->APLectureTypes[0]);
	InitializeLectureType(PLesson->APLectureTypes[0]);

	// since this is the very first Lecture Type of the Lesson, it will be of type (main)
	PLesson->APLectureTypes[0]->PLectureTypeLookup = APLectureTypeLookups[1];
}

void InitializeProgramme(MyProgramme * const PProgramme)
{
	PProgramme->PProgrammeLookup = APProgrammeLookups[0];
	
	PProgramme->APLessons = malloc(sizeof * PProgramme->APLessons);
	PProgramme->APLessons[0] = NULL;
}

void TrimSpacesAndNbsps(const wchar_t ** const start, const wchar_t ** const end)
{
	for (;;)
	{
		if ((*start) < (*end) && (*end)[-1] == L' ')
		{
			(*end)--;
		}
		else if ((*start) < (*end) - 5 && WcsIsWcsWithStartAndEnd(L"&nbsp;", (*end) - 6, (*end)))
		{
			(*end) -= 6;
		}
		else
		{
			break;
		}
	}

	while ((*start) < (*end) && (*start)[0] == L' ')
	{
		(*start)++;
	}
}

size_t AmountofSpacesinWcs(const wchar_t * wcs_start)
{
	while (*wcs_start)
		if (*wcs_start++ == L' ')
			return 1;

	return 0;
}

// finds the ID of TheEntry inside TheLookup
// creates a new entry inside TheLookup if necessary
// returns the ID
MyLookup * FindTheLookup(const wchar_t * const TheEntry_start, const wchar_t * const TheEntry_end, MyLookup *** const PAPLookup)
{
	size_t size = 0;
	for (; (*PAPLookup)[size] != NULL; size++)
		if (WcsIsWcsWithStartAndEnd((*PAPLookup)[size]->Entry, TheEntry_start, TheEntry_end))
			return (*PAPLookup)[size];

	(*PAPLookup) = realloc((*PAPLookup), (size + 2) * sizeof * (*PAPLookup));
	if ((*PAPLookup) == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	(*PAPLookup)[size] = malloc(sizeof * (*PAPLookup)[size]);
	(*PAPLookup)[size]->Entry = wcsdupwithstartandend(TheEntry_start, TheEntry_end);
	(*PAPLookup)[size]->OrderedIndex = 0U;

	(*PAPLookup)[size + 1] = NULL;
	return (*PAPLookup)[size];
}

MyLectureType * FindTheLectureType(const wchar_t * const Start, const wchar_t * const End, MyLectureType *** const PAPLectureTypes)
{
	size_t size = 0;
	MyLookup * PLectureTypeLookup = FindTheLookup(Start, End, &APLectureTypeLookups);

	for (; (*PAPLectureTypes)[size] != NULL; size++)
		if ((*PAPLectureTypes)[size]->PLectureTypeLookup == PLectureTypeLookup)
			return (*PAPLectureTypes)[size];

	(*PAPLectureTypes) = realloc((*PAPLectureTypes), (size + 2) * sizeof * (*PAPLectureTypes));
	if ((*PAPLectureTypes) == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	(*PAPLectureTypes)[size] = malloc(sizeof * (*PAPLectureTypes)[size]);
	if ((*PAPLectureTypes)[size] == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	(*PAPLectureTypes)[size] = malloc(sizeof * (*PAPLectureTypes)[size]);
	InitializeLectureType((*PAPLectureTypes)[size]);
	(*PAPLectureTypes)[size]->PLectureTypeLookup = PLectureTypeLookup;

	(*PAPLectureTypes)[size + 1] = NULL;
	return (*PAPLectureTypes)[size];
}

MyLesson * FindTheLesson(const wchar_t * const Start, const wchar_t * const End, MyLesson *** const PAPLessons)
{
	size_t size = 0;
	MyLookup * PLessonLookup = FindTheLookup(Start, End, &APLessonLookups);

	for (; (*PAPLessons)[size] != NULL; size++)
		if ((*PAPLessons)[size]->PLessonLookup == PLessonLookup)
			return (*PAPLessons)[size];

	(*PAPLessons) = realloc((*PAPLessons), (size + 2) * sizeof * (*PAPLessons));
	if ((*PAPLessons) == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	(*PAPLessons)[size] = malloc(sizeof * (*PAPLessons)[size]);
	if ((*PAPLessons)[size] == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	(*PAPLessons)[size] = malloc(sizeof * (*PAPLessons)[size]);
	InitializeLesson((*PAPLessons)[size]);
	(*PAPLessons)[size]->PLessonLookup = PLessonLookup;

	(*PAPLessons)[size + 1] = NULL;
	return (*PAPLessons)[size];
}

MyProgramme * FindTheProgramme(const wchar_t * const Start, const wchar_t * const End)
{
	size_t size = 0;
	MyLookup * PProgrammeLookup = FindTheLookup(Start, End, &APProgrammeLookups);

	for (; APProgrammes[size] != NULL; size++)
		if (APProgrammes[size]->PProgrammeLookup == PProgrammeLookup)
			return APProgrammes[size];

	APProgrammes = realloc(APProgrammes, (size + 2) * sizeof * APProgrammes);
	if (APProgrammes == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	APProgrammes[size] = malloc(sizeof * APProgrammes[size]);
	InitializeProgramme(APProgrammes[size]);
	APProgrammes[size]->PProgrammeLookup = PProgrammeLookup;

	APProgrammes[size + 1] = NULL;
	return APProgrammes[size];
}

PhraseMatchInAmbiguity FindMatchInAmbiguity(const wchar_t * start, size_t AmbiguousWordCount)
{
	PhraseMatchInAmbiguity result = {
		0, NULL
	};

	const wchar_t * end = start;
	for (; end[0] && AmbiguousWordCount; AmbiguousWordCount--)
	{
		end++;
		while (end[0] && end[0] != L' ')
		{
			end++;
		}
	}

	if (AmbiguousWordCount != 0)
	{
		fwprintf(stderr, L"%d: problem\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	for (size_t NumberofWordsPassed = 0; NumberofWordsPassed < 3; NumberofWordsPassed++)
	{
		for (size_t IDoftheMatch = 0; APClassroomLookups[IDoftheMatch]; IDoftheMatch++)
		{
			if (WcsFoundAtWcsWithStartAndEnd(APClassroomLookups[IDoftheMatch]->Entry, start, end))
			{
				result.NumberofWordsPassed = NumberofWordsPassed;
				result.PLookupForMatch = APClassroomLookups[IDoftheMatch];
				return result;
			}
		}

		while (start[0] != L' ')
		{
			start++;
		}
		while (start[0] == L' ')
		{
			start++;
		}
	}

	return result;
}

void EvaluateRawClassrooms(MyLectureGroup * const PLectureGroup)
{
	const wchar_t * start = PLectureGroup->PRawClassroomsLookup->Entry;

	size_t AtMost = 0U;
	size_t AtLeast = 0U;
	
	size_t NumberofCharacters = 0U;
	size_t NumberofDots = 0U;
	size_t NumberofNumbers = 0U;

	int Ambiguity = 0;

	size_t NumberofFreeCoupleables = 0U;

	for (const wchar_t * helper = start; helper[0];)
	{
		for (; helper[0] && helper[0] != L' '; helper++)
		{
			if (iswalpha(helper[0]))
				NumberofCharacters++;
			else if (iswdigit(helper[0]))
				NumberofNumbers++;
			else if (helper[0] == L'.')
				NumberofDots++;
		}

		/*
		Cdn CDN
		Cdn cdN
		Cdn C1dN1
		Cdn CDn
		Cdn Cdn
		Cdn
		CDn
		C2dN2

		if (rest)											cdn / cDn / cDN
		{													c & (n | D)
			Shouldn't exist according to observations.
		}
		else if (appendable)								Cdn
		{													C & d & n		// cdn is eliminated, C redundant
			NumberofFreeCoupleables++;						d & n
		}
		else
		{
			if (requirespreceeding)							CDN / cdN / C1dN1
			{												(C & D & N) | (c & d & N) | (C == 1 & d & N == 1)	// c tells everything for cdN
				if (NumberofFreeCoupleables)				(C & D & N) | c | (C == 1 & d & N == 1)				// re-ordering
				{											c | (C & D & N) | (C == 1 & d & N == 1)				// C redundant
					NumberofFreeCoupleables--;				c | (D & N) | (C == 1 & d & N == 1)					// d actually redundant, else it'd be a CDN and we already have it
					min++;
					max++;
				}
				else
				{
					Shouldn't be happening according to observations.
				}
			}
			else if (maybepreceeded)						CDn		// D tells them all
			{												D
				NumberofFreeCoupleables++;
			}
			else if (standalone)							C2dN2	// just else is enough
			{
				min++;
				max++;
			}

			if (There is at least one pair & There are even amount of free coupleables)
			{
				Minimum is ambiguous.
			}
			min += NumberofFreeCoupleables / 2 + NumberofFreeCoupleables % 2;
			max += NumberofFreeCoupleables;
			NumberofFreeCoupleables = 0;
		}
		*/

		if (!NumberofCharacters && (!NumberofNumbers || NumberofDots))
		{
			fwprintf(stderr, L"%d: encountered a nonconsidered pattern for a room name in (%ls)\n", __LINE__, PLectureGroup->PRawClassroomsLookup->Entry);
			return;
		}
		else if (!NumberofDots && !NumberofNumbers)
		{
			NumberofFreeCoupleables++;
		}
		else
		{
			if (!NumberofCharacters || (NumberofDots && NumberofNumbers) || (NumberofCharacters == 1 && NumberofNumbers == 1))
			{
				if (NumberofFreeCoupleables)
				{
					NumberofFreeCoupleables--;
					AtLeast++;
					AtMost++;
				}
				else
				{
					fwprintf(stderr, L"%d: encountered a nonconsidered sequence for a room name in (%ls)\n", __LINE__, PLectureGroup->PRawClassroomsLookup->Entry);
					return;
				}
			}
			else if (NumberofDots)
			{
				NumberofFreeCoupleables++;
			}
			else
			{
				AtLeast++;
				AtMost++;
			}

			if (NumberofFreeCoupleables)
			{
				if (NumberofFreeCoupleables / 2 && NumberofFreeCoupleables % 2)
				{
					Ambiguity = 1;
				}
				AtLeast += NumberofFreeCoupleables / 2 + NumberofFreeCoupleables % 2;
				AtMost += NumberofFreeCoupleables;
				NumberofFreeCoupleables = 0;
			}
		}

		NumberofCharacters = 0;
		NumberofDots = 0;
		NumberofNumbers = 0;

		while (helper[0] == L' ')
		{
			helper++;
		}
	}

	if (NumberofFreeCoupleables)
	{
		if (NumberofFreeCoupleables / 2 && NumberofFreeCoupleables % 2)
		{
			Ambiguity = 1;
		}
		AtLeast += NumberofFreeCoupleables / 2 + NumberofFreeCoupleables % 2;
		AtMost += NumberofFreeCoupleables;
		NumberofFreeCoupleables = 0;
	}

	NumberofCharacters = 0;
	NumberofDots = 0;
	NumberofNumbers = 0;

	if (AtMost == PLectureGroup->NumberofLectures)
	{
		MyLecture * PLecture = PLectureGroup->ALectures;
		const wchar_t * helperess = NULL;

		for (const wchar_t * helper = start; helper[0];)
		{
			for (; helper[0] && helper[0] != L' '; helper++)
			{
				if (iswalpha(helper[0]))
					NumberofCharacters++;
				else if (iswdigit(helper[0]))
					NumberofNumbers++;
				else if (helper[0] == L'.')
					NumberofDots++;
			}

			// requires preceeding (will be preceded)
			// may be preceeded (won't be preceeded)
			// must stay alone (won't be preceeded)
			// may be appended (won't be preceeded)

			if (!NumberofCharacters || (NumberofDots && NumberofNumbers) || (NumberofCharacters == 1 && !NumberofDots && NumberofNumbers == 1))
			// requires preceeding
			{
				PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
				PLecture++;
				start = helper;
				while (start[0] == L' ')
				{
					start++;
				}
				helperess = NULL;
			}
			else if (NumberofDots || NumberofNumbers)
			// may be preceded or must stand alone
			{
				if (helperess)
				{
					PLecture->PClassroomLookup = FindTheLookup(start, helperess, &APClassroomLookups);
					PLecture++;
					start = helperess;
					while (start[0] == L' ')
					{
						start++;
					}
					helperess = NULL;
				}
				PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
				PLecture++;
				start = helper;
				while (start[0] == L' ')
				{
					start++;
				}
			}
			else
			// may be appended
			{
				if (helperess)
				{
					PLecture->PClassroomLookup = FindTheLookup(start, helperess, &APClassroomLookups);
					PLecture++;
					start = helperess;
					while (start[0] == L' ')
					{
						start++;
					}
				}
				helperess = helper;
			}

			NumberofCharacters = 0;
			NumberofDots = 0;
			NumberofNumbers = 0;

			while (helper[0] == L' ')
			{
				helper++;
			}
		}

		if (helperess)
		{
			PLecture->PClassroomLookup = FindTheLookup(start, helperess, &APClassroomLookups);
			PLecture++;
			start = helperess;
			while (start[0] == L' ')
			{
				start++;
			}
		}
		PLectureGroup->LectureGroupState = LectureGroupState_Complete;
		return;
	}

	if (AtLeast == PLectureGroup->NumberofLectures)
	{
		if (!Ambiguity)
		{
			MyLecture * PLecture = PLectureGroup->ALectures;
			const wchar_t * helperess = NULL;
			const wchar_t * helpjester = NULL;

			for (const wchar_t * helper = start; helper[0];)
			{
				for (; helper[0] && helper[0] != L' '; helper++)
				{
					if (iswalpha(helper[0]))
						NumberofCharacters++;
					else if (iswdigit(helper[0]))
						NumberofNumbers++;
					else if (helper[0] == L'.')
						NumberofDots++;
				}

				// requires preceeding (will be preceded)
				// may be preceeded (will be preceeded, if available)
				// must stay alone (won't be preceeded)
				// may be appended (will be preceeded, if available and next one isn't 'requires preceeding')

				if (!NumberofCharacters || (NumberofDots && NumberofNumbers) || (NumberofCharacters == 1 && !NumberofDots && NumberofNumbers == 1))
				// requires preceeding
				{
					if (helpjester)
					{
						PLecture->PClassroomLookup = FindTheLookup(start, helpjester, &APClassroomLookups);
						PLecture++;
						start = helpjester;
						while (start[0] == L' ')
						{
							start++;
						}
						helpjester = NULL;
					}
					PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
					PLecture++;
					start = helper;
					while (start[0] == L' ')
					{
						start++;
					}
					helperess = NULL;
				}
				else if (NumberofDots)
				// may be preceded
				{
					if (helperess)
					{
						PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
						PLecture++;
						start = helper;
						while (start[0] == L' ')
						{
							start++;
						}
						helperess = NULL;
					}
					else
					{
						PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
						PLecture++;
						start = helper;
						while (start[0] == L' ')
						{
							start++;
						}
					}
				}
				else if (NumberofNumbers)
				// must stand alone
				{
					if (helpjester)
					{
						PLecture->PClassroomLookup = FindTheLookup(start, helperess, &APClassroomLookups);
						PLecture++;
						start = helperess;
						while (start[0] == L' ')
						{
							start++;
						}
						helpjester = NULL;
						helperess = NULL;
					}
					else if (helperess)
					{
						PLecture->PClassroomLookup = FindTheLookup(start, helperess, &APClassroomLookups);
						PLecture++;
						start = helperess;
						while (start[0] == L' ')
						{
							start++;
						}
						helperess = NULL;
					}

					PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
					PLecture++;
					start = helper;
					while (start[0] == L' ')
					{
						start++;
					}
				}
				else
				// may be appended
				{
					if (helpjester)
					{
						PLecture->PClassroomLookup = FindTheLookup(start, helperess, &APClassroomLookups);
						PLecture++;
						start = helperess;
						while (start[0] == L' ')
						{
							start++;
						}
						helpjester = NULL;
						helperess = helper;
					}
					else if (helperess)
					{
						helpjester = helperess;
						helperess = helper;
					}
					helperess = helper;
				}

				NumberofCharacters = 0;
				NumberofDots = 0;
				NumberofNumbers = 0;

				while (helper[0] == L' ')
				{
					helper++;
				}
			}

			if (helperess)
			{
				PLecture->PClassroomLookup = FindTheLookup(start, helperess, &APClassroomLookups);
				PLecture++;
				start = helperess;
				while (start[0] == L' ')
				{
					start++;
				}
			}

			PLectureGroup->LectureGroupState = LectureGroupState_Complete;
			return;
		}
		else
		{
			// it is ambiguous, but we shall make an educated guess
			// using the data about classrooms we have thus far
			Ambiguity = 0;
			NumberofFreeCoupleables = 0;
			MyLecture * PLecture = PLectureGroup->ALectures;
			const wchar_t * helper;
			
			for (helper = start; helper[0];)
			{
				for (; helper[0] && helper[0] != L' '; helper++)
				{
					if (iswalpha(helper[0]))
						NumberofCharacters++;
					else if (iswdigit(helper[0]))
						NumberofNumbers++;
					else if (helper[0] == L'.')
						NumberofDots++;
				}
				while (helper[0] && helper[1] == L' ')
					helper++;

				if (!NumberofDots && !NumberofNumbers)
				{
					NumberofFreeCoupleables++;
				}
				else
				{
					if (!NumberofCharacters || (NumberofDots && NumberofNumbers) || (NumberofCharacters == 1 && NumberofNumbers == 1))
					{
						NumberofFreeCoupleables--;
						if (NumberofFreeCoupleables == 0)
						{
							PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
							PLecture++;
							start = helper;
							while (start[0] == L' ')
							{
								start++;
							}
						}
					}
					else if (NumberofDots)
					{
						NumberofFreeCoupleables++;
					}
					else
					{
						if (NumberofFreeCoupleables == 0)
						{
							PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
							PLecture++;
							start = helper;
							while (start[0] == L' ')
							{
								start++;
							}
						}
					}

					if (NumberofFreeCoupleables)
					{
						if (NumberofFreeCoupleables / 2 && NumberofFreeCoupleables % 2)
						{
							// case is ambiguous, try to resolve
							PhraseMatchInAmbiguity result = FindMatchInAmbiguity(start, NumberofFreeCoupleables);
							if (result.PLookupForMatch == NULL)
							{
								// ambiguity couldn't be resolved
								return;
							}
							if (result.NumberofWordsPassed)
							{
								for (helper = start; result.NumberofWordsPassed; result.NumberofWordsPassed--)
								{
									while (helper[0] == L' ')
									{
										helper++;
									}
									while (helper[0] != L' ')
									{
										helper++;
									}
								}
								PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
								PLecture++;
								start = helper;
							}
							PLecture->PClassroomLookup = result.PLookupForMatch;
							PLecture++;
							wcsstradvance(start, result.PLookupForMatch->Entry);
							while (start[0] == L' ')
							{
								start++;
							}
						}
						else
						{
							if (NumberofFreeCoupleables == 1)
							{
								helper = start;
								while (helper[0] != L' ')
								{
									helper++;
								}
								PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
								PLecture++;
								start = helper;
								while (start[0] == L' ')
								{
									start++;
								}
							}
							else /*if (NumberofFreeCoupleables % 2 == 0)*/
							{
								for (size_t i = 0; i < NumberofFreeCoupleables / 2; i++)
								{
									helper = start;
									while (helper[0] != L' ')
									{
										helper++;
									}
									while (helper[0] == L' ')
									{
										helper++;
									}
									while (helper[0] != L' ')
									{
										helper++;
									}

									PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
									PLecture++;
									start = helper;
									while (start[0] == L' ')
									{
										start++;
									}
								}
							}
						}
						NumberofFreeCoupleables = 0;
					}
					helper = start;
				}

				NumberofCharacters = 0;
				NumberofDots = 0;
				NumberofNumbers = 0;
			}

			if (NumberofFreeCoupleables)
			{
				if (NumberofFreeCoupleables / 2 && NumberofFreeCoupleables % 2)
				{
					// case is ambiguous, try to resolve
					PhraseMatchInAmbiguity result = FindMatchInAmbiguity(start, NumberofFreeCoupleables);
					if (result.PLookupForMatch == NULL)
					{
						// ambiguity couldn't be resolved
						return;
					}
					if (result.NumberofWordsPassed)
					{
						for (helper = start; result.NumberofWordsPassed; result.NumberofWordsPassed--)
						{
							while (helper[0] == L' ')
							{
								helper++;
							}
							while (helper[0] != L' ')
							{
								helper++;
							}
						}
						PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
						PLecture++;
						start = helper;
					}
					PLecture->PClassroomLookup = result.PLookupForMatch;
					PLecture++;
					wcsstradvance(start, result.PLookupForMatch->Entry);
					while (start[0] == L' ')
					{
						start++;
					}
				}
				else
				{
					if (NumberofFreeCoupleables == 1)
					{
						helper = start;
						while (helper[0] != L' ')
						{
							helper++;
						}
						PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
						PLecture++;
						start = helper;
						while (start[0] == L' ')
						{
							start++;
						}
					}
					else /*if (NumberofFreeCoupleables % 2 == 0)*/
					{
						for (size_t i = 0; i < NumberofFreeCoupleables / 2; i++)
						{
							helper = start;
							while (helper[0] != L' ')
							{
								helper++;
							}
							while (helper[0] == L' ')
							{
								helper++;
							}
							while (helper[0] != L' ')
							{
								helper++;
							}

							PLecture->PClassroomLookup = FindTheLookup(start, helper, &APClassroomLookups);
							PLecture++;
							start = helper;
							while (start[0] == L' ')
							{
								start++;
							}
						}
					}
				}
				NumberofFreeCoupleables = 0;
			}
			PLectureGroup->LectureGroupState = LectureGroupState_Complete;
		}
	}
}

void EvaluateRawSlots(MyLectureGroup * const PLectureGroup)
{
	const wchar_t * start = PLectureGroup->PRawSlotsLookup->Entry;
	const size_t RawSlotsLength = wcslen(start);

	size_t AtMost = RawSlotsLength;
	for (const wchar_t * helper = start; helper[0]; helper++)
	{
		if (!iswdigit(helper[0]))
		{
			// wild character
			return;
		}
		if (helper[0] == L'0')
		{
			if (helper > start && helper[-1] == L'1')
			{
				AtMost--;
			}
			else
			{
				// 0 not preceded by 1
				return;
			}
		}
	}
	if (AtMost == PLectureGroup->NumberofLectures)
	{
		for (size_t i = 0; i < AtMost; i++)
		{
			if (start[0] == L'1' && start[1] == L'0')
			{
				PLectureGroup->ALectures[i].Slot = 10;
				start += 2;
			}
			else
			{
				PLectureGroup->ALectures[i].Slot = start[0] - L'0';
				start++;
			}
		}
		PLectureGroup->LectureGroupState = LectureGroupState_DaysandSlots;
		return;
	}

	size_t AtLeast = 0;
	for (const wchar_t * helper = start; helper[0];)
	{
		size_t NumberofFreeCoupleables = 0U;
		while (helper[0] == L'1')
		{
			NumberofFreeCoupleables++;
			helper++;
		}
		if (helper[0] == L'0')
		{
			NumberofFreeCoupleables--;
			AtLeast++;
			helper++;
		}
		else if (helper[0])
		{
			NumberofFreeCoupleables++;
			helper++;
		}
		
		if (NumberofFreeCoupleables / 2 && NumberofFreeCoupleables % 2)
		{
			// ambiguous
			return;
		}

		AtLeast += NumberofFreeCoupleables / 2 + NumberofFreeCoupleables % 2;
	}
	if (AtLeast == PLectureGroup->NumberofLectures)
	{
		for (size_t i = 0U; i < AtLeast; i++)
		{
			if (start[0] == L'1')
			{
				if (start[1] == L'1' && start[2] == L'0')
				{
					PLectureGroup->ALectures[i].Slot = 1U;
					i++;
					PLectureGroup->ALectures[i].Slot = 10U;
					start += 3;
				}
				else
				{
					PLectureGroup->ALectures[i].Slot = 10U + start[1] - L'0';
					start += 2;
				}
			}
			else
			{
				PLectureGroup->ALectures[i].Slot = start[0] - L'0';
				start++;
			}
		}
		PLectureGroup->LectureGroupState = LectureGroupState_DaysandSlots;
		return;
	}
}

void EvaluateRawDays(MyLectureGroup * const PLectureGroup)
{
	for (const wchar_t * helper = PLectureGroup->PRawDaysLookup->Entry; *helper; helper++)
	{
		switch (*helper)
		{
			case L'M':
			case L'W':
			case L'F':
				PLectureGroup->NumberofLectures++;
				break;
			case L'T':
				if (helper[1] == L'h' || helper[1] == L'H')
				{
					helper++;
				}
				PLectureGroup->NumberofLectures++;
				break;
			case L'S':
				if (helper[1] == L't' || helper[1] == L'T')
				{
					helper++;
				}
				PLectureGroup->NumberofLectures++;
				break;
			default:
				PLectureGroup->LectureGroupState = LectureGroupState_Failed;
				return;
		}
	}

	// allocate array of lectures, initialize Slots and Classrooms with zeroes
	PLectureGroup->ALectures = realloc(PLectureGroup->ALectures, PLectureGroup->NumberofLectures * sizeof * PLectureGroup->ALectures);
	for (size_t i = 0; i < PLectureGroup->NumberofLectures; i++)
	{
		PLectureGroup->ALectures[i].Slot = 0U;
		PLectureGroup->ALectures[i].PClassroomLookup = APClassroomLookups[0];
	}

	MyLecture * PLecture = PLectureGroup->ALectures;

	for (const wchar_t * helper = PLectureGroup->PRawDaysLookup->Entry; *helper; helper++, PLecture++)
	{
		switch (*helper)
		{
			case L'M':
				PLecture->Day = Monday;
				break;
			case L'W':
				PLecture->Day = Wednesday;
				break;
			case L'F':
				PLecture->Day = Friday;
				break;
			case L'T':
				if (helper[1] == L'h' || helper[1] == L'H')
				{
					helper++;
					PLecture->Day = Thursday;
				}
				else
				{
					PLecture->Day = Tuesday;
				}
				break;
			case L'S':
				if (helper[1] == L't' || helper[1] == L'T')
				{
					helper++;
					PLecture->Day = Saturday;
				}
				else
				{
					PLecture->Day = Sunday;
				}
				break;
		}
	}

	PLectureGroup->LectureGroupState = LectureGroupState_OnlyDays;
}

size_t FindCellIndexWithinRow(const wchar_t * RowStart, const wchar_t * RowEnd, const wchar_t * const Matchwcs)
{
	const wchar_t * MatchStart = wcsstr(RowStart, Matchwcs);
	if (MatchStart == NULL || MatchStart > RowEnd)
	{
		fwprintf(stderr, L"%d: no such string (%ls) within row\n", __LINE__, Matchwcs);
		return 0;
	}

	// no need to be too excessive, we will assume
	// RowStart -- MatchStart -- RowEnd
	// and that row properly has cells within

	RowStart = wcsstr(RowStart, L"</td>");
	size_t CellIndex;
	for (CellIndex = 1; RowStart < MatchStart; CellIndex++)
	{
		RowStart = wcsstr(RowStart + 1, L"</td>");
	}

	return CellIndex;
}

void PrintLookupIntoFile(FILE * const LookupFile, const MyLookup * const * PFirstLookupP)
{
	fwprintf(LookupFile, L"%ls", (*PFirstLookupP)->Entry);
	for (PFirstLookupP++; *PFirstLookupP; PFirstLookupP++)
	{
		fwprintf(LookupFile, L"|%ls", (*PFirstLookupP)->Entry);
	}
}

/*
typedef struct MyLookupTag
{
	wchar_t * Entry;
	size_t OrderedIndex;
} MyLookup;

typedef enum
{
	Monday = 1,
	Tuesday,
	Wednesday,
	Thursday,
	Friday,
	Saturday,
	Sunday
} DAYS;

typedef struct MyLectureTag
{
	DAYS Day;
	unsigned int Slot;
	MyLookup * PClassroomLookup;
} MyLecture;

typedef enum MyLectureGroupStateTag
{
	LectureGroupState_Undefined,
	LectureGroupState_TBA,
	LectureGroupState_Failed,
	LectureGroupState_OnlyDays,
	LectureGroupState_DaysandSlots,
	LectureGroupState_Complete
} MyLectureGroupState;

typedef struct MyLectureGroupTag
{
	MyLookup * PSectionLookup;
	MyLookup * PInstructorLookup;
	MyLookup * PRawDaysLookup;
	MyLookup * PRawSlotsLookup;
	MyLookup * PRawClassroomsLookup;
	MyLectureGroupState LectureGroupState;
	size_t NumberofLectures;
	MyLecture * ALectures;
} MyLectureGroup;

typedef struct MyLectureGroupTypeTag
{
	MyLookup * PLectureTypeLookup;
	MyLectureGroup ** APLectureGroups;
} MyLectureType;

typedef struct MyLessonTag
{
	MyLookup * PLessonLookup;
	MyLookup * PLessonNameLookup;
	MyLookup * PCreditsLookup;
	MyLookup * PLinkLookup;
	MyLectureType ** APLectureTypes;
} MyLesson;

typedef struct MyProgrammeTag
{
	MyLookup * PProgrammeLookup;
	MyLesson ** APLessons;
} MyProgramme;
*/

void PrintProgrammesIntoFile(FILE * const ProgrammesFile)
{
	/*
	|1 a1 ProgrammeLookupIndex
	   b1 c1 LessonLookupIndex c2 LessonNameLookupIndex c3 CreditsLookupIndex c4 LinkLookupIndex
	      d1 e1 LectureTypeLookupIndex
		     f1 g1 SectionLookupIndex g2 InstructorLookupIndex g3 RawDaysLookupIndex g4 RawSlotsLookupIndex g5 RawClassroomsLookupIndex g6 LectureGroupState
		        h1 i1 Day i2 Slot i3 ClassroomLookupIndex
	
	#1: |
	#2: a
	#3: b
	#4: c
	#5: d
	#6: e
	#7: f
	#8: g
	#9: h
	#10:i
	*/
	for (const MyProgramme * const * PFirstProgrammeP = APProgrammes; *PFirstProgrammeP; PFirstProgrammeP++)
	{
		fwprintf(ProgrammesFile, L"|a%u",
				 (*PFirstProgrammeP)->PProgrammeLookup->OrderedIndex
				 );
		for (const MyLesson * const * PFirstLessonP = (*PFirstProgrammeP)->APLessons; *PFirstLessonP; PFirstLessonP++)
		{
			fwprintf(ProgrammesFile, L"bc%uc%uc%uc%u",
					 (*PFirstLessonP)->PLessonLookup->OrderedIndex,
					 (*PFirstLessonP)->PLessonNameLookup->OrderedIndex,
					 (*PFirstLessonP)->PCreditsLookup->OrderedIndex,
					 (*PFirstLessonP)->PLinkLookup->OrderedIndex
					 );
			for (const MyLectureType * const * PFirstLectureTypeP = (*PFirstLessonP)->APLectureTypes; *PFirstLectureTypeP; PFirstLectureTypeP++)
			{
				fwprintf(ProgrammesFile, L"de%u",
						 (*PFirstLectureTypeP)->PLectureTypeLookup->OrderedIndex // 1 has a special meaning, indicates (main)
						 );
				for (const MyLectureGroup * const * PFirstLectureGroupP = (*PFirstLectureTypeP)->APLectureGroups; *PFirstLectureGroupP; PFirstLectureGroupP++)
				{
					fwprintf(ProgrammesFile, L"fg%ug%ug%ug%ug%ug%u",
							 (*PFirstLectureGroupP)->PSectionLookup->OrderedIndex,
							 (*PFirstLectureGroupP)->PInstructorLookup->OrderedIndex,
							 (*PFirstLectureGroupP)->PRawDaysLookup->OrderedIndex,
							 (*PFirstLectureGroupP)->PRawSlotsLookup->OrderedIndex,
							 (*PFirstLectureGroupP)->PRawClassroomsLookup->OrderedIndex,
							 (*PFirstLectureGroupP)->LectureGroupState
							 );
					for (size_t LectureIndex = 0; LectureIndex < (*PFirstLectureGroupP)->NumberofLectures; LectureIndex++)
					{
						fwprintf(ProgrammesFile, L"hi%ui%ui%u",
								 (*PFirstLectureGroupP)->ALectures[LectureIndex].Day,
								 (*PFirstLectureGroupP)->ALectures[LectureIndex].Slot,
								 (*PFirstLectureGroupP)->ALectures[LectureIndex].PClassroomLookup->OrderedIndex
								 );
					}
				}
			}
		}
	}
}

int MatchCellContents(const wchar_t * RowStart, const wchar_t * RowEnd, const size_t CellIndex, const wchar_t ** const PCellStart, const wchar_t ** const PCellEnd)
{
	if (RowEnd == NULL)
	{
		fwprintf(stderr, L"%d: such row has no end\n", __LINE__);
		return 0;
	}

	RowStart = wcsstr(RowStart, L"<td");
	for (size_t i = 1; i < CellIndex; i++)
	{
		RowStart = wcsstr(RowStart + 1, L"<td");
		if (RowStart == NULL || RowStart >= RowEnd)
		{
			fwprintf(stderr, L"%d: there is no cell #%u within current row (%u)\n", __LINE__, CellIndex, i);
			return 0;
		}
	}

	if (MatchFromRightToLeft(RowStart, L">", L"</", PCellStart, PCellEnd) == NULL)
	{
		fwprintf(stderr, L"%d: cell contents could not be grabbed\n", __LINE__);
		return 0;
	}
	TrimSpacesAndNbsps(PCellStart, PCellEnd);

	return 1;
}

void InterpretDepartmentPage(const MemoryStruct * const data, MyLookup * const PLinkLookup)
{
	const wchar_t * RowStart	= data->memory;
	const wchar_t * RowEnd		= NULL;
	const wchar_t * CellStart	= NULL;
	const wchar_t * CellEnd		= NULL;
	const wchar_t * helper		= NULL;


	// Column Index Lookup creation
	RowStart = wcsstradvance(RowStart, L"<tr class=\"schtitle");
	if (RowStart == NULL)
	{
		fwprintf(stderr, L"%d: title row missing in %ls\n", __LINE__, PLinkLookup->Entry);
		return;
	}
	RowStart = wcsstradvance(RowStart, L">");
	if (RowStart == NULL)
	{
		fwprintf(stderr, L"%d: title row missing in %ls\n", __LINE__, PLinkLookup->Entry);
		return;
	}
	RowEnd = wcsstr(RowStart, L"</tr>");
	if (RowEnd == NULL)
	{
		fwprintf(stderr, L"%d: title missed ending tag in %ls\n", __LINE__, PLinkLookup->Entry);
		return;
	}
	ColumnIndexLookup columnindexlookup;
	columnindexlookup.CodeSec	= FindCellIndexWithinRow(RowStart, RowEnd, L"Code.Sec");
	columnindexlookup.Name		= FindCellIndexWithinRow(RowStart, RowEnd, L"Name");
	columnindexlookup.Cr		= FindCellIndexWithinRow(RowStart, RowEnd, L"Cr.");
	columnindexlookup.Instr		= FindCellIndexWithinRow(RowStart, RowEnd, L"Instr.");
	columnindexlookup.Days		= FindCellIndexWithinRow(RowStart, RowEnd, L"Days");
	columnindexlookup.Hours		= FindCellIndexWithinRow(RowStart, RowEnd, L"Hours");
	columnindexlookup.Rooms		= FindCellIndexWithinRow(RowStart, RowEnd, L"Rooms");

	MyProgramme * PProgramme = NULL;
	MyLesson * PLesson = NULL;
	MyLookup * PSectionLookup = NULL;

	for (RowStart = wcsstradvance(RowStart, L"<tr class=\"schtd"); RowStart; RowStart = wcsstradvance(RowStart, L"<tr class=\"schtd"))
	{
		RowEnd = wcsstr(RowStart, L"</tr>");
		if (RowEnd == NULL)
		{
			fwprintf(stderr, L"%d: one row missed ending tag in %ls\n", __LINE__, PLinkLookup->Entry);
			return;
		}

		// Code.Sec cell
		if (!MatchCellContents(RowStart, RowEnd, columnindexlookup.CodeSec, &CellStart, &CellEnd))
		{
			fwprintf(stderr, L"%d: problem\n", __LINE__);
			exit(EXIT_FAILURE);
		}
		
		MyLectureType * PLectureType = NULL;
		MyLectureGroup * PLectureGroup = NULL;

		if (CellStart == CellEnd)	// means that it is a sublesson (ps, lab, etc.)
		{
			if (PProgramme == NULL)
			{
				fwprintf(stderr, L"%d: problem\n", __LINE__);
				exit(EXIT_FAILURE);
			}

			// Name cell
			if (!MatchCellContents(RowStart, RowEnd, columnindexlookup.Name, &CellStart, &CellEnd))
			{
				fwprintf(stderr, L"%d: problem\n", __LINE__);
				exit(EXIT_FAILURE);
			}
			PLectureType = FindTheLectureType(CellStart, CellEnd, &PLesson->APLectureTypes);
		}
		else
		{
			helper = CellStart;
			while (iswalpha(helper[0]))
			{
				if (++helper >= CellEnd)
				{
					fwprintf(stderr, L"%d: Code.Sec cell ended too soon\n", __LINE__);
					exit(EXIT_FAILURE);
				}
			}
			PProgramme = FindTheProgramme(CellStart, helper);

			while (helper[0] == L' ')
			{
				if (++helper >= CellEnd)
				{
					fwprintf(stderr, L"%d: Code.Sec cell ended too soon\n", __LINE__);
					exit(EXIT_FAILURE);
				}
			}
			CellStart = helper;
			while (helper[0] != L'.')
			{
				if (++helper >= CellEnd)
				{
					fwprintf(stderr, L"%d: Code.Sec cell ended too soon\n", __LINE__);
					exit(EXIT_FAILURE);
				}
			}
			PLesson = FindTheLesson(CellStart, helper, &PProgramme->APLessons);

			helper++;
			if (helper >= CellEnd)
			{
				fwprintf(stderr, L"%d: Code.Sec cell ended too soon\n", __LINE__);
				exit(EXIT_FAILURE);
			}
			PSectionLookup = FindTheLookup(helper, CellEnd, &APSectionLookups);

			// Name cell
			if (!MatchCellContents(RowStart, RowEnd, columnindexlookup.Name, &CellStart, &CellEnd))
			{
				fwprintf(stderr, L"%d: problem\n", __LINE__);
				exit(EXIT_FAILURE);
			}
			PLesson->PLessonNameLookup = FindTheLookup(CellStart, CellEnd, &APLessonNameLookups);

			// Cr. cell
			if (!MatchCellContents(RowStart, RowEnd, columnindexlookup.Cr, &CellStart, &CellEnd))
			{
				fwprintf(stderr, L"%d: problem\n", __LINE__);
				exit(EXIT_FAILURE);
			}
			PLesson->PCreditsLookup = FindTheLookup(CellStart, CellEnd, &APCreditsLookups);

			PLesson->PLinkLookup = PLinkLookup;
			PLectureType = PLesson->APLectureTypes[0];
		}

		PLectureGroup = OneMoreLectureGroup(&PLectureType->APLectureGroups);
		PLectureGroup->PSectionLookup = PSectionLookup;

		// Instr. cell
		if (!MatchCellContents(RowStart, RowEnd, columnindexlookup.Instr, &CellStart, &CellEnd))
		{
			fwprintf(stderr, L"%d: problem\n", __LINE__);
			exit(EXIT_FAILURE);
		}
		PLectureGroup->PInstructorLookup = FindTheLookup(CellStart, CellEnd, &APInstructorLookups);

		// Days cell
		if (!MatchCellContents(RowStart, RowEnd, columnindexlookup.Days, &CellStart, &CellEnd))
		{
			fwprintf(stderr, L"%d: problem\n", __LINE__);
			exit(EXIT_FAILURE);
		}
		PLectureGroup->PRawDaysLookup = FindTheLookup(CellStart, CellEnd, &APRawDaysLookups);

		// Hours cell
		if (!MatchCellContents(RowStart, RowEnd, columnindexlookup.Hours, &CellStart, &CellEnd))
		{
			fwprintf(stderr, L"%d: problem\n", __LINE__);
			exit(EXIT_FAILURE);
		}
		PLectureGroup->PRawSlotsLookup = FindTheLookup(CellStart, CellEnd, &APRawSlotsLookups);

		// Rooms cell
		if (!MatchCellContents(RowStart, RowEnd, columnindexlookup.Rooms, &CellStart, &CellEnd))
		{
			fwprintf(stderr, L"%d: problem\n", __LINE__);
			exit(EXIT_FAILURE);
		}
		PLectureGroup->PRawClassroomsLookup = FindTheLookup(CellStart, CellEnd, &APRawClassroomsLookups);

		// parsing of raw days, slots and classrooms
		if (wcscmp(PLectureGroup->PRawDaysLookup->Entry, L"TBA") == 0)
		{
			PLectureGroup->LectureGroupState = LectureGroupState_TBA;
		}
		else
		{
			EvaluateRawDays(PLectureGroup);
			if (PLectureGroup->LectureGroupState == LectureGroupState_Failed)
			{
				fwprintf(stderr, (PLectureType->PLectureTypeLookup == APLectureTypeLookups[1]) ?
						 L"%d: %ls %ls.%ls%ls parsing failure on D (%ls)\n" :
						 L"%d: %ls %ls.%ls (%ls) parsing failure on D (%ls)\n", __LINE__,
						 PProgramme->PProgrammeLookup->Entry,
						 PLesson->PLessonLookup->Entry,
						 PLectureGroup->PSectionLookup->Entry,
						 (PLectureType->PLectureTypeLookup == APLectureTypeLookups[1]) ? L"" : PLectureType->PLectureTypeLookup->Entry,
						 PLectureGroup->PRawDaysLookup->Entry);
				continue;
			}

			EvaluateRawSlots(PLectureGroup);
			if (PLectureGroup->LectureGroupState == LectureGroupState_OnlyDays)
			{
				fwprintf(stderr, (PLectureType->PLectureTypeLookup == APLectureTypeLookups[1]) ?
						 L"%d: %ls %ls.%ls%ls parsing failure on S (%ls), %u lectures\n" :
						 L"%d: %ls %ls.%ls (%ls) parsing failure on S (%ls), %u lectures\n", __LINE__,
						 PProgramme->PProgrammeLookup->Entry,
						 PLesson->PLessonLookup->Entry,
						 PLectureGroup->PSectionLookup->Entry,
						 (PLectureType->PLectureTypeLookup == APLectureTypeLookups[1]) ? L"" : PLectureType->PLectureTypeLookup->Entry,
						 PLectureGroup->PRawSlotsLookup->Entry,
						 PLectureGroup->NumberofLectures);
				continue;
			}

			EvaluateRawClassrooms(PLectureGroup);
			if (PLectureGroup->LectureGroupState == LectureGroupState_DaysandSlots)
			{
				fwprintf(stderr, (PLectureType->PLectureTypeLookup == APLectureTypeLookups[1]) ?
						 L"%d: %ls %ls.%ls%ls parsing failure on C (%ls), %u lectures\n" :
						 L"%d: %ls %ls.%ls (%ls) parsing failure on C (%ls), %u lectures\n", __LINE__,
						 PProgramme->PProgrammeLookup->Entry,
						 PLesson->PLessonLookup->Entry,
						 PLectureGroup->PSectionLookup->Entry,
						 (PLectureType->PLectureTypeLookup == APLectureTypeLookups[1]) ? L"" : PLectureType->PLectureTypeLookup->Entry,
						 PLectureGroup->PRawClassroomsLookup->Entry,
						 PLectureGroup->NumberofLectures);
				continue;
			}
		}
	}
}

void InterpretSemesterNavigationPage(const MemoryStruct * const data)
{
	const wchar_t * datacaret	= data->memory;
	const wchar_t * start		= NULL;
	const wchar_t * end			= NULL;

	const size_t BaseLinkLength = wcslen(BaseLink);

	for (datacaret = MatchFromLeftToRight(datacaret, L"href=\"", L"\"", &start, &end); datacaret; datacaret = MatchFromLeftToRight(datacaret, L"href=\"", L"\"", &start, &end))
	{
		if (wcswithstartandendstr(start, end, L"bolum="))
		{
			const size_t ADepartmentPageLinkLength = BaseLinkLength + (end - start);
			wchar_t * ADepartmentPageLink = malloc((ADepartmentPageLinkLength + 1) * sizeof * ADepartmentPageLink);
			if (ADepartmentPageLink == NULL)
			{
				fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
				exit(EXIT_FAILURE);
			}
			wcscpy(ADepartmentPageLink, BaseLink);
			wcscpywithstartandend(ADepartmentPageLink + BaseLinkLength, start, end);

			// the returned ID is not useful for anything, therefore ignored
			FindTheLookup(ADepartmentPageLink, ADepartmentPageLink + ADepartmentPageLinkLength, &APLinkLookups);

			free(ADepartmentPageLink);
		}
	}
}

// it returns a wcs which contains the link to the
// latest Semester Navigation Page
wchar_t * InterpretSemesterSelectionPage(const MemoryStruct * const data)
{
	const wchar_t * datacaret = data->memory;

	const size_t BaseLinkLength = wcslen(BaseLink);

	const wchar_t * SublinkStart = NULL;
	const wchar_t * SublinkEnd = NULL;
	datacaret = MatchFromLeftToRight(datacaret, L"<FORM ACTION=\'", L"\'", &SublinkStart, &SublinkEnd);
	if (datacaret == NULL)
	{
		fwprintf(stderr, L"%d: problem\n", __LINE__);
		exit(EXIT_FAILURE);
	}
	const size_t SublinkLength = SublinkEnd - SublinkStart;

	const wchar_t * InputNameStart = NULL;
	const wchar_t * InputNameEnd = NULL;
	datacaret = MatchFromLeftToRight(datacaret, L"<select name=\'", L"\'", &InputNameStart, &InputNameEnd);
	if (datacaret == NULL)
	{
		fwprintf(stderr, L"%d: problem\n", __LINE__);
		exit(EXIT_FAILURE);
	}
	const size_t InputNameLength = InputNameEnd - InputNameStart;

	const wchar_t * InputValueStart = NULL;
	const wchar_t * InputValueEnd = NULL;
	datacaret = MatchFromLeftToRight(datacaret, L"<option value=\'", L"\'", &InputValueStart, &InputValueEnd);
	if (datacaret == NULL)
	{
		fwprintf(stderr, L"%d: problem\n", __LINE__);
		exit(EXIT_FAILURE);
	}
	const size_t InputValueLength = InputValueEnd - InputValueStart;

	wchar_t * const SemesterNavigationPageLink = malloc((BaseLinkLength + SublinkLength + 1 /*?*/ + InputNameLength + 1 /*=*/ + InputValueLength + 1) * sizeof * SemesterNavigationPageLink);

	// BaseLink
	wcscpy(SemesterNavigationPageLink, BaseLink);

	// BaseLinkSublink
	wcscpywithstartandend(SemesterNavigationPageLink + BaseLinkLength, SublinkStart, SublinkEnd);

	// BaseLinkSublink?~~~~~~~~~~
	SemesterNavigationPageLink[BaseLinkLength + SublinkLength] = L'?';

	// BaseLinkSublink?InputName
	wcscpywithstartandend(SemesterNavigationPageLink + BaseLinkLength + SublinkLength + 1, InputNameStart, InputNameEnd);

	// BaseLinkSublink?InputName=~~~~~~~~~~
	SemesterNavigationPageLink[BaseLinkLength + SublinkLength + 1 + InputNameLength] = L'=';

	// BaseLinkSublink?InputName=InputValue
	wcscpywithstartandend(SemesterNavigationPageLink + BaseLinkLength + SublinkLength + 1 + InputNameLength + 1, InputValueStart, InputValueEnd);

	return SemesterNavigationPageLink;
}

size_t writefunction(const void * const delivery, const size_t size, const size_t count, MemoryStruct * const storage)
{
	storage->memory = realloc(storage->memory, (storage->count + count) * sizeof * storage->memory);
	if (storage->memory == NULL)
	{
		fwprintf(stderr, L"%d: not enough memory (realloc returned NULL)\n", __LINE__);
		exit(EXIT_FAILURE);
	}

	mbstowcs(storage->memory + storage->count - 1, delivery, count);
	storage->count += count;
	storage->memory[storage->count - 1] = 0;

	return count;
}

void * MemoryStructReinitialize(MemoryStruct * const storage)
{
	storage->count = 1;
	storage->memory = realloc(storage->memory, sizeof * storage->memory);
	if (storage->memory)
	{
		*storage->memory = 0;
	}
	else
	{
		fwprintf(stderr, L"%d: not enough memory (realloc returned NULL)\n", __LINE__);
		exit(EXIT_FAILURE);
	}
	return storage->memory;
}

void InitializeLookup(MyLookup *** const PAPLookup, const size_t AmountofEmpties)
{
	(*PAPLookup) = malloc((AmountofEmpties + 1) * sizeof * (*PAPLookup));
	if ((*PAPLookup) == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}
	(*PAPLookup)[AmountofEmpties] = NULL;

	// Amount of Empties many lookup entries from start are reserved for various reasons
	// Common reasons:
	// [0] ... initial/default/erroneous value
	// [1] ... special meaning

	for (size_t i = 0; i < AmountofEmpties; i++)
	{
		(*PAPLookup)[i] = malloc(sizeof * (*PAPLookup)[i]);
		if ((*PAPLookup)[i] == NULL)
		{
			fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
			exit(EXIT_FAILURE);
		}

		(*PAPLookup)[i]->Entry = malloc(sizeof * (*PAPLookup)[i]->Entry);
		if ((*PAPLookup)[i]->Entry == NULL)
		{
			fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
			exit(EXIT_FAILURE);
		}
		(*PAPLookup)[i]->Entry[0] = L'\0';

		(*PAPLookup)[i]->OrderedIndex = 0U;
	}
}

int CompareLookups(const void * Lookup1, const void * Lookup2)
{
	wchar_t * Entry1 = (*((MyLookup **) Lookup1))->Entry;
	wchar_t * Entry2 = (*((MyLookup **) Lookup2))->Entry;
	return wcscmp(Entry1, Entry2);
}

void SortArrayofLookups(MyLookup ** const APLookups)
{
	size_t size = 0;
	while (APLookups[size] != NULL)
		size++;

	qsort(APLookups, size, sizeof * APLookups, CompareLookups);

	for (size_t i = 0; i < size; i++)
		APLookups[i]->OrderedIndex = i;
}

void InitializeGlobals(void)
{
	InitializeLookup(&APProgrammeLookups, 1);
	InitializeLookup(&APLessonLookups, 1);
	InitializeLookup(&APLessonNameLookups, 1);
	InitializeLookup(&APCreditsLookups, 1);
	InitializeLookup(&APLinkLookups, 1);
	InitializeLookup(&APLectureTypeLookups, 2);
	// 2, because ID = 1 will be used for main type of lectures
	InitializeLookup(&APSectionLookups, 1);
	InitializeLookup(&APInstructorLookups, 1);
	InitializeLookup(&APRawDaysLookups, 1);
	InitializeLookup(&APRawSlotsLookups, 1);
	InitializeLookup(&APRawClassroomsLookups, 1);
	InitializeLookup(&APClassroomLookups, 1);

	APProgrammes = malloc(sizeof * APProgrammes);
	if (APProgrammes == NULL)
	{
		fwprintf(stderr, L"%d: allocation failure\n", __LINE__);
		exit(EXIT_FAILURE);
	}
	APProgrammes[0] = NULL;
}

size_t FindLongestEntryLength(const MyLookup * const * PFirstLookupP)
{
	size_t LongestEntryLength = 0U;
	for (; *PFirstLookupP; PFirstLookupP++)
	{
		LongestEntryLength = max(wcslen(PFirstLookupP[0]->Entry), LongestEntryLength);
	}
	return LongestEntryLength;
}

void InitializeSomeExtremes(DAYS * const FirstDay, DAYS * const LastDay, unsigned int * const FirstSlot, unsigned int * const LastSlot)
{
	for (const MyProgramme * const * PFirstProgrammeP = APProgrammes; *PFirstProgrammeP; PFirstProgrammeP++)
	{
		for (const MyLesson * const * PFirstLessonP = (*PFirstProgrammeP)->APLessons; *PFirstLessonP; PFirstLessonP++)
		{
			for (const MyLectureType * const * PFirstLectureTypeP = (*PFirstLessonP)->APLectureTypes; *PFirstLectureTypeP; PFirstLectureTypeP++)
			{
				for (const MyLectureGroup * const * PFirstLectureGroupP = (*PFirstLectureTypeP)->APLectureGroups; *PFirstLectureGroupP; PFirstLectureGroupP++)
				{
					if ((*PFirstLectureGroupP)->NumberofLectures > 0 && (*PFirstLectureGroupP)->LectureGroupState >= LectureGroupState_DaysandSlots)
					{
						*FirstDay = PFirstLectureGroupP[0]->ALectures[0].Day;
						*LastDay = PFirstLectureGroupP[0]->ALectures[0].Day;
						*FirstSlot = PFirstLectureGroupP[0]->ALectures[0].Slot;
						*LastSlot = PFirstLectureGroupP[0]->ALectures[0].Slot;
						return;
					}
				}
			}
		}
	}
}

void PrintExtremesIntoFile(FILE * ExtremesFile)
{
	DAYS FirstDay;
	DAYS LastDay;
	unsigned int FirstSlot;
	unsigned int LastSlot;

	InitializeSomeExtremes(&FirstDay, &LastDay, &FirstSlot, &LastSlot);

	for (const MyProgramme * const * PFirstProgrammeP = APProgrammes; *PFirstProgrammeP; PFirstProgrammeP++)
	{
		for (const MyLesson * const * PFirstLessonP = (*PFirstProgrammeP)->APLessons; *PFirstLessonP; PFirstLessonP++)
		{
			for (const MyLectureType * const * PFirstLectureTypeP = (*PFirstLessonP)->APLectureTypes; *PFirstLectureTypeP; PFirstLectureTypeP++)
			{
				for (const MyLectureGroup * const * PFirstLectureGroupP = (*PFirstLectureTypeP)->APLectureGroups; *PFirstLectureGroupP; PFirstLectureGroupP++)
				{
					for (size_t LectureIndex = 0; LectureIndex < (*PFirstLectureGroupP)->NumberofLectures; LectureIndex++)
					{
						if (PFirstLectureGroupP[0]->ALectures[LectureIndex].Day < FirstDay)
							FirstDay = PFirstLectureGroupP[0]->ALectures[LectureIndex].Day;
						if (PFirstLectureGroupP[0]->ALectures[LectureIndex].Day > LastDay)
							LastDay = PFirstLectureGroupP[0]->ALectures[LectureIndex].Day;
						if ((*PFirstLectureGroupP)->LectureGroupState >= LectureGroupState_DaysandSlots)
						{
							if (PFirstLectureGroupP[0]->ALectures[LectureIndex].Slot < FirstSlot)
								FirstSlot = PFirstLectureGroupP[0]->ALectures[LectureIndex].Slot;
							if (PFirstLectureGroupP[0]->ALectures[LectureIndex].Slot > LastSlot)
								LastSlot = PFirstLectureGroupP[0]->ALectures[LectureIndex].Slot;
						}
					}
				}
			}
		}
	}

	fwprintf(ExtremesFile, L"%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u|%u",
			 (unsigned int) FirstDay,
			 (unsigned int) LastDay,
			 FirstSlot,
			 LastSlot,
			 FindLongestEntryLength(APProgrammeLookups),
			 FindLongestEntryLength(APLessonLookups),
			 FindLongestEntryLength(APLessonNameLookups),
			 FindLongestEntryLength(APCreditsLookups),
			 FindLongestEntryLength(APLinkLookups),
			 FindLongestEntryLength(APLectureTypeLookups),
			 FindLongestEntryLength(APSectionLookups),
			 FindLongestEntryLength(APInstructorLookups),
			 FindLongestEntryLength(APRawDaysLookups),
			 FindLongestEntryLength(APRawSlotsLookups),
			 FindLongestEntryLength(APRawClassroomsLookups),
			 FindLongestEntryLength(APClassroomLookups)
			 );
}

int main(void)
{
	time_t TimeStart = time(NULL);
	clock_t ClockStart = clock( );

	InitializeGlobals( );
	setlocale(LC_ALL, "tr-TR");

	FILE * ErrorLog = _wfreopen(L"error.log", L"w, ccs=UNICODE", stderr);
	if (ErrorLog == NULL)
	{
		fwprintf(stderr, L"%d: problem\n", __LINE__);
	}

	CURL * curl;
	CURLcode res;
	char URL[2048];

	MemoryStruct storage = {
		0,
		NULL
	};
	MemoryStructReinitialize(&storage);

	if (curl_global_init(CURL_GLOBAL_ALL))
	{
		fwprintf(stderr, L"%d: something went wrong with curl_global_init( )\n", __LINE__);
		return EXIT_FAILURE;
	}

	curl = curl_easy_init( );
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &storage);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunction);


		// Semester Selection Page: the one with drop-down list to select semester
		URL[wcstombs(URL, SemesterSelectionPageLink, sizeof URL / sizeof * URL - 1)] = 0;
		curl_easy_setopt(curl, CURLOPT_URL, URL);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
		{
			fwprintf(stderr, L"%d: curl_easy_perform( ) failed: %hs\n", __LINE__, curl_easy_strerror(res));
			return EXIT_FAILURE;
		}

		wchar_t * SemesterNavigationPageLink = InterpretSemesterSelectionPage(&storage);
		MemoryStructReinitialize(&storage);
		wprintf(L"%ls\n", SemesterNavigationPageLink);


		// Semester Navigation Page: the one with 2-column table full with links
		URL[wcstombs(URL, SemesterNavigationPageLink, sizeof URL / sizeof * URL - 1)] = 0;
		free(SemesterNavigationPageLink);
		SemesterNavigationPageLink = NULL;
		curl_easy_setopt(curl, CURLOPT_URL, URL);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
		{
			fwprintf(stderr, L"%d: curl_easy_perform( ) failed: %hs\n", __LINE__, curl_easy_strerror(res));
			return EXIT_FAILURE;
		}

		InterpretSemesterNavigationPage(&storage);
		MemoryStructReinitialize(&storage);

		// LookupID = 0 is reserved as a default value for structure elements
		for (size_t LinkIndex = 1; APLinkLookups[LinkIndex]; LinkIndex++)
		{
			URL[wcstombs(URL, APLinkLookups[LinkIndex]->Entry, sizeof URL / sizeof * URL - 1)] = 0;
			curl_easy_setopt(curl, CURLOPT_URL, URL);
			res = curl_easy_perform(curl);
			if (res != CURLE_OK)
			{
				fwprintf(stderr, L"%d: curl_easy_perform( ) failed: %hs\n", __LINE__, curl_easy_strerror(res));
				return EXIT_FAILURE;
			}

			InterpretDepartmentPage(&storage, APLinkLookups[LinkIndex]);
			MemoryStructReinitialize(&storage);
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	else
	{
		fwprintf(stderr, L"%d: something went wrong with curl_easy_init( )\n", __LINE__);
		return EXIT_FAILURE;
	}

	SortArrayofLookups(APProgrammeLookups);
	SortArrayofLookups(APLessonLookups);
	SortArrayofLookups(APLessonNameLookups);
	SortArrayofLookups(APCreditsLookups);
	SortArrayofLookups(APLinkLookups);
	SortArrayofLookups(APLectureTypeLookups);
	SortArrayofLookups(APSectionLookups);
	SortArrayofLookups(APInstructorLookups);
	SortArrayofLookups(APRawDaysLookups);
	SortArrayofLookups(APRawSlotsLookups);
	SortArrayofLookups(APRawClassroomsLookups);
	SortArrayofLookups(APClassroomLookups);

	FILE * ProgrammeLookupFile		= _wfopen(L"ProgrammeLookup.txt",		L"w, ccs=UNICODE");
	FILE * LessonLookupFile			= _wfopen(L"LessonLookup.txt",			L"w, ccs=UNICODE");
	FILE * LessonNameLookupFile		= _wfopen(L"LessonNameLookup.txt",		L"w, ccs=UNICODE");
	FILE * CreditsLookupFile		= _wfopen(L"CreditsLookup.txt",			L"w, ccs=UNICODE");
	FILE * LinkLookupFile			= _wfopen(L"LinkLookup.txt",			L"w, ccs=UNICODE");
	FILE * LectureTypeLookupFile	= _wfopen(L"LectureTypeLookup.txt",		L"w, ccs=UNICODE");
	FILE * SectionLookupFile		= _wfopen(L"SectionLookup.txt",			L"w, ccs=UNICODE");
	FILE * InstructorLookupFile		= _wfopen(L"InstructorLookup.txt",		L"w, ccs=UNICODE");
	FILE * RawDaysLookupFile		= _wfopen(L"RawDaysLookup.txt",			L"w, ccs=UNICODE");
	FILE * RawSlotsLookupFile		= _wfopen(L"RawSlotsLookup.txt",		L"w, ccs=UNICODE");
	FILE * RawClassroomsLookupFile	= _wfopen(L"RawClassroomsLookup.txt",	L"w, ccs=UNICODE");
	FILE * ClassroomLookupFile		= _wfopen(L"ClassroomLookup.txt",		L"w, ccs=UNICODE");

	FILE * ProgrammesArrayFile		= _wfopen(L"ProgrammesArray.txt",		L"w, ccs=UNICODE");
	FILE * ExtremesFile				= _wfopen(L"Extremes.txt",				L"w, ccs=UNICODE");
	if (!ProgrammeLookupFile || !LessonLookupFile || !LessonNameLookupFile || !CreditsLookupFile || !LinkLookupFile || !LectureTypeLookupFile || !SectionLookupFile ||
		!InstructorLookupFile || !RawDaysLookupFile || !RawSlotsLookupFile || !RawClassroomsLookupFile || !ClassroomLookupFile || !ProgrammesArrayFile || !ExtremesFile)
	{
		fwprintf(stderr, L"%d: problem\n", __LINE__);
		return EXIT_FAILURE;
	}

	PrintLookupIntoFile(ProgrammeLookupFile, APProgrammeLookups);
	fclose(ProgrammeLookupFile);
	ProgrammeLookupFile = NULL;

	PrintLookupIntoFile(LessonLookupFile, APLessonLookups);
	fclose(LessonLookupFile);
	LessonLookupFile = NULL;

	PrintLookupIntoFile(LessonNameLookupFile, APLessonNameLookups);
	fclose(LessonNameLookupFile);
	LessonNameLookupFile = NULL;

	PrintLookupIntoFile(CreditsLookupFile, APCreditsLookups);
	fclose(CreditsLookupFile);
	CreditsLookupFile = NULL;

	PrintLookupIntoFile(LinkLookupFile, APLinkLookups);
	fclose(LinkLookupFile);
	LinkLookupFile = NULL;

	PrintLookupIntoFile(LectureTypeLookupFile, APLectureTypeLookups);
	fclose(LectureTypeLookupFile);
	LectureTypeLookupFile = NULL;

	PrintLookupIntoFile(SectionLookupFile, APSectionLookups);
	fclose(SectionLookupFile);
	SectionLookupFile = NULL;

	PrintLookupIntoFile(InstructorLookupFile, APInstructorLookups);
	fclose(InstructorLookupFile);
	InstructorLookupFile = NULL;

	PrintLookupIntoFile(RawDaysLookupFile, APRawDaysLookups);
	fclose(RawDaysLookupFile);
	RawDaysLookupFile = NULL;

	PrintLookupIntoFile(RawSlotsLookupFile, APRawSlotsLookups);
	fclose(RawSlotsLookupFile);
	RawSlotsLookupFile = NULL;

	PrintLookupIntoFile(RawClassroomsLookupFile, APRawClassroomsLookups);
	fclose(RawClassroomsLookupFile);
	RawClassroomsLookupFile = NULL;

	PrintLookupIntoFile(ClassroomLookupFile, APClassroomLookups);
	fclose(ClassroomLookupFile);
	ClassroomLookupFile = NULL;

	PrintProgrammesIntoFile(ProgrammesArrayFile);
	fclose(ProgrammesArrayFile);
	ProgrammesArrayFile = NULL;

	PrintExtremesIntoFile(ExtremesFile);
	fclose(ExtremesFile);
	ExtremesFile = NULL;

	FILE * UpdateInformationFile = _wfopen(L"UpdateInformation.txt", L"w, ccs=UNICODE");
	if (!UpdateInformationFile)
	{
		fwprintf(stderr, L"%d: problem\n", __LINE__);
		return EXIT_FAILURE;
	}

	clock_t ClockEnd = clock( );
	wchar_t * UpdateTime = _wasctime(localtime(&TimeStart));
	fwprintf(UpdateInformationFile, L"%.*ls\"%ld", wcslen(UpdateTime) - 1, UpdateTime, ClockEnd - ClockStart);

	fclose(UpdateInformationFile);
	UpdateInformationFile = NULL;

	wprintf(L"execution has been completed in %ld milliseconds\n", ClockEnd - ClockStart);
	fwprintf(stderr, L"%d: execution has been completed in %ld milliseconds\n", __LINE__, ClockEnd - ClockStart);

	if (ErrorLog)
	{
		fclose(ErrorLog);
		ErrorLog = NULL;
	}

	return 0;
}