/*! \addtogroup Imgent
\{ */ 
//! \file Text_Lines.cpp \brief Implementation file for KTextLines class

#include "StdAfx.h"
#include "Text_Lines.h"
#include "Entity_Utils.h"
#include <list>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace std;


#define MARK_DELETE(pEntity) \
	if (KTextLines::toDelete == NULL) KTextLines::toDelete = new KEntityPointersArray(); \
	KTextLines::toDelete->Add(pEntity);


bool KTextLines::DebugEnabled = false;
char* KTextLines::DebugOutputPath = ".\\";
char* KTextLines::DebugFilenamePrefix = "text-lines";
KEntityPointersArray* KTextLines::toDelete = NULL;


void KTextLines::DoCleanup()
{
	if (toDelete != NULL)
	{
		toDelete->DestroyAllEntities();
		delete toDelete;
		toDelete = NULL;
	}
}


void KTextLines::BuildLines(KEntityPointersArray& letters, /*OUT*/ KEntityPointersArray& lines)
{
	if (letters.GetSize() <= 0) return;
	ASSERT(&letters != &lines); // source and destination can't be the same!

	lines.RemoveAll();

	KEntityPointersArray* mediumEntities = new KEntityPointersArray();
	KEntityPointersArray* smallEntities = new KEntityPointersArray();
	KEntityPointersArray* bigEntities = new KEntityPointersArray();

	KEntityUtils::FilterBySize(letters, *mediumEntities, *smallEntities, 5, 7);

	float avg_width = KEntityUtils::GetAverageWidth(*mediumEntities, KEntityUtils::ARITHMETIC_MEAN);
	float avg_height = KEntityUtils::GetAverageHeight(*mediumEntities, KEntityUtils::ARITHMETIC_MEAN);

	KEntityUtils::FilterBySize(*mediumEntities, *mediumEntities, *bigEntities, 0, 0, 5 * avg_width, 5 * avg_height);

	avg_width = KEntityUtils::GetAverageWidth(*mediumEntities, KEntityUtils::ARITHMETIC_MEAN);
	avg_height = KEntityUtils::GetAverageHeight(*mediumEntities, KEntityUtils::ARITHMETIC_MEAN);

	list<KGenericEntity*> entities;

	for (int i = 0; i < mediumEntities->GetSize(); ++i) 
		entities.push_back((KGenericEntity*) mediumEntities->GetAt(i));
	mediumEntities->RemoveAll();

	float local_avg_width, local_max_height, epsi;
	KGenericEntity* seed;
	KEntityCollection* line;
	bool left_found, right_found;
	KPageRectangle left_rect, right_rect;
	int children;

	while (!entities.empty())
	{
		seed = entities.front();
		entities.pop_front();

		line = new KEntityCollection(seed->ImagePageOwner);
		line->AddChild(seed);
		children = 1;
		local_max_height = seed->boundingRectangle.Height();

		do
		{
			left_found = false;
			right_found = false;

			KPageRectangle& curr_rect = line->boundingRectangle;
			local_avg_width = curr_rect.Width() / children;

			//epsi = max(0.1f, 1.1f - 0.1f * children);
			epsi = 1.0f;
			if (curr_rect.Height() > 2 * local_max_height) epsi = 0.1f;

			left_rect.left = curr_rect.left - 3 * local_avg_width;
			left_rect.right = curr_rect.left;
			left_rect.top = curr_rect.top - 0.75f * local_max_height * epsi;
			left_rect.bottom = curr_rect.bottom + 0.75f * local_max_height * epsi;

			right_rect.right = curr_rect.right + 3 * local_avg_width;
			right_rect.left = curr_rect.right;
			right_rect.top = curr_rect.top - 0.75f * local_max_height * epsi;
			right_rect.bottom = curr_rect.bottom + 0.75f * local_max_height * epsi;

			list<KGenericEntity*>::iterator ent_it = entities.begin();
			while (ent_it != entities.end())
			{
				KPageRectangle& ent_rect = (*ent_it)->boundingRectangle;

				if (left_rect.Include(ent_rect))
				{
					left_found = true;
					line->AddChild(*ent_it);
					ent_it = entities.erase(ent_it);
					++children;
					if (local_max_height < ent_rect.Height())
						local_max_height = ent_rect.Height();
					continue;
				}

				if (right_rect.Include(ent_rect))
				{
					right_found = true;
					line->AddChild(*ent_it);
					ent_it = entities.erase(ent_it);
					++children;
					if (local_max_height < ent_rect.Height())
						local_max_height = ent_rect.Height();
					continue;
				}

				++ent_it;
			}
		}
		while (left_found || right_found);

		if (children > 1) // > 3
		{
			MARK_DELETE(line);
			lines.Add(line);
		}
		else
		{
			line->RemoveAllChildren();
			delete line;
			smallEntities->Add(seed);
			//MARK_DELETE(line);
			//smallEntities->Add(line);
		}
	}

	entities.clear();
	for (int i = 0; i < smallEntities->GetSize(); ++i) 
		entities.push_back((KGenericEntity*) smallEntities->GetAt(i));
	smallEntities->RemoveAll();

	KPageRectangle curr_rect;
	for (int i = 0; i < lines.GetSize(); ++i)
	{
		line = (KEntityCollection*) lines.GetAt(i);

		KPageRectangle& ent_rect = line->boundingRectangle;
		curr_rect.left = ent_rect.left - ent_rect.Height();
		curr_rect.right = ent_rect.right + ent_rect.Height();
		curr_rect.top = ent_rect.top - 0.25f * ent_rect.Height();
		curr_rect.bottom = ent_rect.bottom + 0.25f * ent_rect.Height();

		list<KGenericEntity*>::iterator ent_it = entities.begin();
		while (ent_it != entities.end())
		{
			if (curr_rect.Include((*ent_it)->boundingRectangle))
			{
				line->AddChild(*ent_it);
				ent_it = entities.erase(ent_it);
			}
			else
				++ent_it;
		}
	}

	while (!entities.empty())
	{
		line = new KEntityCollection(entities.front()->ImagePageOwner);
		MARK_DELETE(line);
		line->AddChild(entities.front());
		lines.Add(line);
		entities.pop_front();
	}

	for (int i = 0; i < bigEntities->GetSize(); ++i) 
		entities.push_back((KGenericEntity*) bigEntities->GetAt(i));
	bigEntities->RemoveAll();


	delete mediumEntities;
	delete smallEntities;
	delete bigEntities;
}

/*  \} */