<?php

class Modules_FriendlyLogs_CustomButtons extends pm_Hook_CustomButtons
{

    public function getButtons() // Functions must return list of list with our buttons properties
    {
        return [[
            'place' => self::PLACE_DOMAIN_PROPERTIES,
            'title' => 'Friendly logs',
            'description' => 'Button to call friendly logs extension',
            'link' => pm_Context::getActionUrl('index', 'index'),
        ]];
    }

}
